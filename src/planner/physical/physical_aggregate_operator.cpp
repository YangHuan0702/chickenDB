//
// Created by huan.yang on 2026-05-21.
//
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_aggregate.h"
#include "planner/physical/agg/physical_hash_aggregate.h"
#include "executor/expression_evaluator.h"
#include "executor/chunk_util.h"

using namespace chickenDB;

auto Planner::PhysicalAggregateOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::AGGREGATE,
                                      "[Planner] Physical Aggregate Operator handler error,target logical operator is not Aggregate type.");
    auto *logical_agg = dynamic_cast<LogicalAggregate *>(logical_operator.get());
    return std::make_unique<PhysicalHashAggregateOperator>(logical_agg->group_cols_, logical_agg->agg_col_,
                                                           logical_agg->agg_func_);
}


auto PhysicalHashAggregateOperator::Init() -> void {
    Child(0)->Init();
    hash_table_.clear();
    is_built_ = false;
}

auto PhysicalHashAggregateOperator::Close() -> void {
    Child(0)->Close();
}

// 两阶段：首次 Next 把孩子读干、按 group key 聚合；随后一次性输出所有组。
auto PhysicalHashAggregateOperator::Next() -> Chunk * {
    if (!is_built_) {
        std::vector<size_t> group_idx;
        std::vector<ColumnType> group_types;
        size_t agg_idx = 0;
        bool resolved = false;

        while (Chunk *in = Child(0)->Next()) {
            if (!resolved) {
                auto col_map = ChunkUtil::BuildColMap(*in);
                for (col_id_t cid : col_ids_) {
                    auto it = col_map.find(cid);
                    ChickenException::AssertCondition(it != col_map.end(),
                                                      "[HashAgg] group column not found");
                    group_idx.push_back(it->second);
                    group_types.push_back(in->GetColumn(it->second).GetType());
                }
                auto ait = col_map.find(agg_col_);
                ChickenException::AssertCondition(ait != col_map.end(),
                                                  "[HashAgg] aggregate column not found");
                agg_idx = ait->second;
                ChickenException::AssertCondition(!in->GetColumn(agg_idx).IsVar(),
                                                  "[HashAgg] aggregate on varchar column not supported");
                resolved = true;
            }

            const size_t n = in->Count();
            for (size_t r = 0; r < n; r++) {
                std::string key = ChunkUtil::RowKey(*in, r, group_idx);
                auto &entry = hash_table_[key];
                if (entry.group_vals.empty() && entry.group_strs.empty() && !group_idx.empty()) {
                    entry.group_vals.resize(group_idx.size(), 0.0);
                    entry.group_strs.resize(group_idx.size());
                    for (size_t gpos = 0; gpos < group_idx.size(); gpos++) {
                        const Vector &gv = in->GetColumn(group_idx[gpos]);
                        if (gv.IsVar()) {
                            entry.group_strs[gpos] = std::string(gv.GetString(r));
                        } else {
                            entry.group_vals[gpos] = gv.GetType() == ColumnType::NUMBER
                                                         ? static_cast<double>(gv.GetValue<int32_t>(r))
                                                         : gv.GetValue<double>(r);
                        }
                    }
                }
                const Vector &av = in->GetColumn(agg_idx);
                double val = av.GetType() == ColumnType::NUMBER
                                 ? static_cast<double>(av.GetValue<int32_t>(r))
                                 : av.GetValue<double>(r);
                entry.state.Update(val);
            }
        }

        // 物化所有组到一个输出 chunk：group 列(保持原类型)... + 聚合结果列。
        // 聚合结果列类型：COUNT 为 NUMBER（整数计数），其余为 DOUBLE。
        const size_t num_groups = hash_table_.empty() ? 1 : hash_table_.size();
        const bool agg_is_count = (agg_func_ == AggFuncType::COUNT);
        std::vector<ColumnType> out_types;
        for (size_t i = 0; i < col_ids_.size(); i++) {
            // 若已解析到原始 group 列类型则沿用（保留 VARCHAR），否则退化为 DOUBLE。
            out_types.push_back(i < group_types.size() ? group_types[i] : ColumnType::DOUBLE);
        }
        out_types.push_back(agg_is_count ? ColumnType::NUMBER : ColumnType::DOUBLE);
        output_.Init(out_types, num_groups);

        std::vector<col_id_t> out_ids = col_ids_;
        out_ids.push_back(agg_col_); // 聚合结果列复用聚合列 id
        output_.SetColIds(out_ids);

        size_t row = 0;
        for (auto &kv : hash_table_) {
            for (size_t g = 0; g < col_ids_.size(); g++) {
                Vector &gcol = output_.GetColumn(g);
                if (gcol.IsVar()) {
                    gcol.AppendString(kv.second.group_strs[g]);
                } else {
                    gcol.SetValue<double>(row, kv.second.group_vals[g]);
                }
            }
            const double res = kv.second.state.Result(agg_func_);
            if (agg_is_count) {
                output_.GetColumn(col_ids_.size()).SetValue<int32_t>(row, static_cast<int32_t>(res));
            } else {
                output_.GetColumn(col_ids_.size()).SetValue<double>(row, res);
            }
            row++;
        }
        output_.SetCount(row);
        is_built_ = true;
        return row > 0 ? &output_ : nullptr;
    }

    // 已输出过，结束。
    return nullptr;
}

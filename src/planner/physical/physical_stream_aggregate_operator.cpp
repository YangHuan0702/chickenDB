//
// Created by huan.yang on 2026-06-11.
//
#include "planner/physical/agg/physical_stream_aggregate.h"

#include <string>
#include <vector>

#include "common/chicken_execption.h"
#include "executor/chunk_util.h"

using namespace chickenDB;

auto PhysicalStreamAggregateOperator::Init() -> void {
    Child(0)->Init();
    built_ = false;
}

auto PhysicalStreamAggregateOperator::Close() -> void {
    Child(0)->Close();
}

// 流式：输入按 group_by 有序，逐行扫描，key 变化即结算上一组。
// 这里把所有组物化到一个输出 chunk（语义等价，验证用）。
auto PhysicalStreamAggregateOperator::Next() -> Chunk * {
    if (built_) {
        return nullptr;
    }

    std::vector<size_t> group_idx;
    size_t agg_idx = 0;
    bool resolved = false;

    std::string cur_key;
    bool has_group = false;
    std::vector<double> cur_group_vals;
    AggregateState cur_state;

    // 收集结果行：每组 -> group_vals + sum + count。
    std::vector<std::vector<double>> result_groups;
    std::vector<double> result_sum;
    std::vector<int64_t> result_count;

    auto flush_group = [&]() {
        if (!has_group) return;
        result_groups.push_back(cur_group_vals);
        result_sum.push_back(cur_state.sum);
        result_count.push_back(cur_state.count);
    };

    while (Chunk *in = Child(0)->Next()) {
        if (!resolved) {
            auto col_map = ChunkUtil::BuildColMap(*in);
            for (col_id_t cid : group_by_) {
                auto it = col_map.find(cid);
                ChickenException::AssertCondition(it != col_map.end(), "[StreamAgg] group column not found");
                group_idx.push_back(it->second);
            }
            auto ait = col_map.find(agg_col_);
            ChickenException::AssertCondition(ait != col_map.end(), "[StreamAgg] aggregate column not found");
            agg_idx = ait->second;
            // StreamAgg 输出 group 列固定为 DOUBLE，本期不支持 varchar group/agg 列
            // （HashAggregate 支持 group by varchar）。
            for (size_t gi : group_idx) {
                ChickenException::AssertCondition(!in->GetColumn(gi).IsVar(),
                    "[StreamAgg] varchar group column not supported");
            }
            ChickenException::AssertCondition(!in->GetColumn(agg_idx).IsVar(),
                "[StreamAgg] varchar aggregate column not supported");
            resolved = true;
        }

        const size_t n = in->Count();
        for (size_t r = 0; r < n; r++) {
            std::string key = ChunkUtil::RowKey(*in, r, group_idx);
            if (!has_group || key != cur_key) {
                flush_group();
                cur_key = key;
                has_group = true;
                cur_state.Reset();
                cur_group_vals.clear();
                for (size_t gi : group_idx) {
                    const Vector &gv = in->GetColumn(gi);
                    cur_group_vals.push_back(gv.GetType() == ColumnType::NUMBER
                                                 ? static_cast<double>(gv.GetValue<int32_t>(r))
                                                 : gv.GetValue<double>(r));
                }
            }
            const Vector &av = in->GetColumn(agg_idx);
            double val = av.GetType() == ColumnType::NUMBER
                             ? static_cast<double>(av.GetValue<int32_t>(r))
                             : av.GetValue<double>(r);
            cur_state.Update(val);
        }
    }
    flush_group();

    const size_t num_groups = result_groups.empty() ? 1 : result_groups.size();
    std::vector<ColumnType> out_types;
    for (size_t i = 0; i < group_by_.size(); i++) out_types.push_back(ColumnType::DOUBLE);
    out_types.push_back(ColumnType::DOUBLE); // sum
    out_types.push_back(ColumnType::NUMBER); // count
    output_.Init(out_types, num_groups);

    std::vector<col_id_t> out_ids = group_by_;
    out_ids.push_back(agg_col_);
    out_ids.push_back(-1);
    output_.SetColIds(out_ids);

    for (size_t row = 0; row < result_groups.size(); row++) {
        for (size_t g = 0; g < group_by_.size(); g++) {
            output_.GetColumn(g).SetValue<double>(row, result_groups[row][g]);
        }
        output_.GetColumn(group_by_.size()).SetValue<double>(row, result_sum[row]);
        output_.GetColumn(group_by_.size() + 1)
            .SetValue<int32_t>(row, static_cast<int32_t>(result_count[row]));
    }
    output_.SetCount(result_groups.size());
    built_ = true;
    return result_groups.empty() ? nullptr : &output_;
}

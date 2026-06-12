//
// Created by huan.yang on 2026-05-21.
//
#include <algorithm>
#include <string>
#include <vector>

#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_join.h"
#include "planner/physical/join/physical_nested_loop_join.h"
#include "planner/physical/join/physical_hash_join.h"
#include "planner/physical/join/physical_merge_join.h"
#include "planner/physical/join/physical_index_nl_join.h"
#include "executor/join_util.h"
#include "index/hash_index.h"

using namespace chickenDB;

namespace {
    // 把一行 double 在指定 key 列上序列化成哈希键。
    auto KeyOf(const std::vector<double> &row, const std::vector<size_t> &key_idx) -> std::string {
        std::string k;
        k.reserve(key_idx.size() * sizeof(double));
        for (size_t idx : key_idx) {
            double v = row[idx];
            k.append(reinterpret_cast<const char *>(&v), sizeof(double));
        }
        return k;
    }
}

auto Planner::PhysicalJoinOperator(std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    ChickenException::AssertCondition(logical_operator->type_ == LogicalOperatorType::JOIN,
                                      "[Planner] target logical operator is not Join type.");
    auto *logical_join = dynamic_cast<LogicalJoin *>(logical_operator.get());
    // 默认哈希连接（等值连接最稳）。左右 key 来自 ON 条件解析。
    return std::make_unique<PhysicalHashJoin>(logical_join->left_keys_, logical_join->right_keys_);
}


// ---- NestedLoopJoin ----
auto PhysicalNestedLoopJoin::Init() -> void {
    Child(0)->Init();
    Child(1)->Init();
    right_built_ = false;
    right_rows_.clear();
}

auto PhysicalNestedLoopJoin::Close() -> void {
    Child(0)->Close();
    Child(1)->Close();
}

auto PhysicalNestedLoopJoin::Next() -> Chunk * {
    if (right_built_) {
        return nullptr; // 一次性产出全部结果
    }
    // 物化右表。
    JoinRows right = JoinUtil::Materialize(Child(1), right_keys_);
    right_types_ = right.types;
    right_ids_ = right.col_ids;
    right_key_idx_ = right.key_idx;

    // 物化左表。
    JoinRows left = JoinUtil::Materialize(Child(0), left_keys_);

    std::vector<ColumnType> out_types;
    std::vector<col_id_t> out_ids;
    JoinUtil::BuildOutputSchema(left.types, left.col_ids, right.types, right.col_ids, out_types, out_ids);

    // 先数匹配行数以分配容量。
    std::vector<std::pair<size_t, size_t>> matches;
    for (size_t l = 0; l < left.rows.size(); l++) {
        for (size_t r = 0; r < right.rows.size(); r++) {
            if (JoinUtil::KeysEqual(left.rows[l], left.key_idx, right.rows[r], right.key_idx)) {
                matches.emplace_back(l, r);
            }
        }
    }

    output_.Init(out_types, matches.empty() ? 1 : matches.size());
    output_.SetColIds(out_ids);
    for (size_t i = 0; i < matches.size(); i++) {
        JoinUtil::EmitJoined(output_, i, left.rows[matches[i].first], right.rows[matches[i].second], out_types);
    }
    output_.SetCount(matches.size());
    right_built_ = true;
    return matches.empty() ? nullptr : &output_;
}


// ---- HashJoin ----
auto PhysicalHashJoin::Init() -> void {
    Child(0)->Init();
    Child(1)->Init();
    built_ = false;
    hash_table_.clear();
    build_rows_.clear();
}

auto PhysicalHashJoin::Close() -> void {
    Child(0)->Close();
    Child(1)->Close();
}

auto PhysicalHashJoin::Next() -> Chunk * {
    if (built_) {
        return nullptr;
    }
    // build 侧（左）物化 + 建表。
    JoinRows build = JoinUtil::Materialize(Child(0), left_keys_);
    build_rows_ = build.rows;
    build_types_ = build.types;
    build_ids_ = build.col_ids;
    build_key_idx_ = build.key_idx;
    for (size_t i = 0; i < build_rows_.size(); i++) {
        hash_table_[KeyOf(build_rows_[i], build_key_idx_)].push_back(i);
    }

    // probe 侧（右）物化。
    JoinRows probe = JoinUtil::Materialize(Child(1), right_keys_);

    std::vector<ColumnType> out_types;
    std::vector<col_id_t> out_ids;
    JoinUtil::BuildOutputSchema(build_types_, build_ids_, probe.types, probe.col_ids, out_types, out_ids);

    std::vector<std::pair<size_t, size_t>> matches;
    for (size_t p = 0; p < probe.rows.size(); p++) {
        std::string key = KeyOf(probe.rows[p], probe.key_idx);
        auto it = hash_table_.find(key);
        if (it == hash_table_.end()) continue;
        for (size_t b : it->second) {
            matches.emplace_back(b, p);
        }
    }

    output_.Init(out_types, matches.empty() ? 1 : matches.size());
    output_.SetColIds(out_ids);
    for (size_t i = 0; i < matches.size(); i++) {
        JoinUtil::EmitJoined(output_, i, build_rows_[matches[i].first], probe.rows[matches[i].second], out_types);
    }
    output_.SetCount(matches.size());
    built_ = true;
    return matches.empty() ? nullptr : &output_;
}


// ---- MergeJoin ----
auto PhysicalMergeJoin::Init() -> void {
    Child(0)->Init();
    Child(1)->Init();
    built_ = false;
}

auto PhysicalMergeJoin::Close() -> void {
    Child(0)->Close();
    Child(1)->Close();
}

auto PhysicalMergeJoin::Next() -> Chunk * {
    if (built_) {
        return nullptr;
    }
    JoinRows left = JoinUtil::Materialize(Child(0), left_keys_);
    JoinRows right = JoinUtil::Materialize(Child(1), right_keys_);

    // 两侧各自按 key 排序（不假设输入已序，保证正确性）。
    auto cmp = [](const std::vector<size_t> &kidx) {
        return [kidx](const std::vector<double> &a, const std::vector<double> &b) {
            for (size_t i : kidx) {
                if (a[i] < b[i]) return true;
                if (a[i] > b[i]) return false;
            }
            return false;
        };
    };
    std::sort(left.rows.begin(), left.rows.end(), cmp(left.key_idx));
    std::sort(right.rows.begin(), right.rows.end(), cmp(right.key_idx));

    std::vector<ColumnType> out_types;
    std::vector<col_id_t> out_ids;
    JoinUtil::BuildOutputSchema(left.types, left.col_ids, right.types, right.col_ids, out_types, out_ids);

    // 双指针归并 + 等值块笛卡尔积。
    std::vector<std::pair<size_t, size_t>> matches;
    size_t i = 0, j = 0;
    auto key_cmp = [](const std::vector<double> &a, const std::vector<size_t> &ak,
                      const std::vector<double> &b, const std::vector<size_t> &bk) -> int {
        for (size_t t = 0; t < ak.size(); t++) {
            if (a[ak[t]] < b[bk[t]]) return -1;
            if (a[ak[t]] > b[bk[t]]) return 1;
        }
        return 0;
    };
    while (i < left.rows.size() && j < right.rows.size()) {
        int c = key_cmp(left.rows[i], left.key_idx, right.rows[j], right.key_idx);
        if (c < 0) { i++; }
        else if (c > 0) { j++; }
        else {
            size_t je = j;
            while (je < right.rows.size() &&
                   key_cmp(left.rows[i], left.key_idx, right.rows[je], right.key_idx) == 0) je++;
            size_t ie = i;
            while (ie < left.rows.size() &&
                   key_cmp(left.rows[ie], left.key_idx, right.rows[j], right.key_idx) == 0) ie++;
            for (size_t li = i; li < ie; li++)
                for (size_t rj = j; rj < je; rj++)
                    matches.emplace_back(li, rj);
            i = ie; j = je;
        }
    }

    output_.Init(out_types, matches.empty() ? 1 : matches.size());
    output_.SetColIds(out_ids);
    for (size_t m = 0; m < matches.size(); m++) {
        JoinUtil::EmitJoined(output_, m, left.rows[matches[m].first], right.rows[matches[m].second], out_types);
    }
    output_.SetCount(matches.size());
    built_ = true;
    return matches.empty() ? nullptr : &output_;
}


// ---- IndexNLJoin：build 侧建哈希索引，probe 每行用 join key 点查（替代 O(n^2) 嵌套循环） ----
auto PhysicalIndexNLJoin::Init() -> void {
    Child(0)->Init();
    Child(1)->Init();
    built_ = false;
}

auto PhysicalIndexNLJoin::Close() -> void {
    Child(0)->Close();
    Child(1)->Close();
}

auto PhysicalIndexNLJoin::Next() -> Chunk * {
    if (built_) {
        return nullptr;
    }
    JoinRows left = JoinUtil::Materialize(Child(0), left_keys_);   // build 侧
    JoinRows right = JoinUtil::Materialize(Child(1), right_keys_); // probe 侧

    // 在 build 侧 join key 上建哈希索引：IndexKey -> build 行下标（编码进 RID.row_idx）。
    HashIndex build_index;
    for (size_t l = 0; l < left.rows.size(); l++) {
        IndexKey key(JoinUtil::KeyVals(left.rows[l], left.key_idx));
        build_index.Insert(key, RID(0, static_cast<uint32_t>(l)));
    }

    std::vector<ColumnType> out_types;
    std::vector<col_id_t> out_ids;
    JoinUtil::BuildOutputSchema(left.types, left.col_ids, right.types, right.col_ids, out_types, out_ids);

    // probe：每行用 join key 点查 build 索引，命中行拼接。
    std::vector<std::pair<size_t, size_t>> matches;
    for (size_t r = 0; r < right.rows.size(); r++) {
        IndexKey key(JoinUtil::KeyVals(right.rows[r], right.key_idx));
        for (const RID &rid : build_index.Find(key)) {
            matches.emplace_back(static_cast<size_t>(rid.row_idx), r);
        }
    }

    output_.Init(out_types, matches.empty() ? 1 : matches.size());
    output_.SetColIds(out_ids);
    for (size_t i = 0; i < matches.size(); i++) {
        JoinUtil::EmitJoined(output_, i, left.rows[matches[i].first], right.rows[matches[i].second], out_types);
    }
    output_.SetCount(matches.size());
    built_ = true;
    return matches.empty() ? nullptr : &output_;
}

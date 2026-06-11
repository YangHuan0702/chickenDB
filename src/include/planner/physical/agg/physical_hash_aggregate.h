//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "aggregate_state.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    /**
     * HashAgg不关心顺序，用哈希表同时维护所有组的聚合状态，全部扫完才输出。内存是 O(G)（G = 组数），可能需要 spill to disk。
     */
    class PhysicalHashAggregateOperator : public PhysicalOperator {
    public:
        explicit PhysicalHashAggregateOperator(std::vector<col_id_t> col_ids,col_id_t agg_col) :
        PhysicalOperator(PhysicalOperatorType::HashAggregate),col_ids_(std::move(col_ids)),agg_col_(agg_col) {}

        ~PhysicalHashAggregateOperator() override = default;

        auto Init() -> void override;

        auto Next() -> Chunk * override;

        auto Close() -> void override;


        std::vector<col_id_t> col_ids_{}; // GROUP BY 列
        col_id_t agg_col_;                // 聚合列

        // group key 序列化 -> 聚合状态 + 该组的 group-by 列值（double，用于输出）
        struct GroupEntry {
            AggregateState state;
            std::vector<double> group_vals;
        };
        std::unordered_map<std::string, GroupEntry> hash_table_;

        bool is_built_{false};
        std::unordered_map<std::string, GroupEntry>::iterator emit_iter_;

    private:
        Chunk output_;
        // 输出列：group_by 各列(DOUBLE) + sum(DOUBLE) + count(NUMBER)。
    };

}

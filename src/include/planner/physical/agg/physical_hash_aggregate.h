//
// Created by huan.yang on 2026-05-09.
//
#pragma once
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


        std::vector<col_id_t> col_ids_{};
        col_id_t agg_col_;

        std::unordered_map<std::string,AggregateState> hash_table_;

        bool is_built_{false};
        std::unordered_map<std::string,AggregateState>::iterator emit_iter_;
    };

}

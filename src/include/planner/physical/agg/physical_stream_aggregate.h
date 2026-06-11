//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "aggregate_state.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    /**
     * 流式聚合：前提是底层数据已经按Group By的列排序好了。算子只需维护当前组的状态，遇到新组就输出老组的结果。
     * StreamAgg利用已有的顺序，维护一个"当前组"的状态，逐行扫描，key 变化时立即输出上一组并重置状态。内存开销是 O(1)，但要求输入有序。
     */
    class PhysicalStreamAggregateOperator : public PhysicalOperator {
    public:
        explicit PhysicalStreamAggregateOperator(col_id_t agg_id, std::vector<col_id_t> group_by) :
        PhysicalOperator(PhysicalOperatorType::StreamAggregate),group_by_(std::move(group_by)),agg_col_(agg_id) {}
        ~PhysicalStreamAggregateOperator() override = default;

        auto Init() -> void override;

        auto Next() -> Chunk * override;

        auto Close() -> void override;

        std::vector<col_id_t> group_by_;
        col_id_t agg_col_;

    private:
        // 一次性聚合所有有序输入，物化全部组到 output_（实现简单且足够验证语义）。
        Chunk output_;
        bool built_{false};
    };

}

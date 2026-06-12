//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "logical_operator.h"
#include "common/types.h"
#include "planner/physical/agg/aggregate_state.h"

namespace chickenDB {

    class LogicalAggregate : public LogicalOperator {
    public:
        explicit LogicalAggregate( ) : LogicalOperator(LogicalOperatorType::AGGREGATE) {}
        ~LogicalAggregate() override = default;

        std::vector<col_id_t> group_cols_; // GROUP BY 列
        col_id_t agg_col_{0};              // 聚合列
        AggFuncType agg_func_{AggFuncType::SUM}; // 聚合函数类型
    };

}

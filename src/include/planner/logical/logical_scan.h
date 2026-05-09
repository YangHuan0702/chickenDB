//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <vector>

#include "logical_operator.h"
#include "binder/expression/bound_expression.h"
#include "common/types.h"

namespace chickenDB {
    class LogicalScan : public LogicalOperator {
    public:
        explicit LogicalScan(table_id_t table_id) : LogicalOperator(LogicalOperatorType::SCAN), table_id_(table_id) {
        }
        ~LogicalScan() override = default;

        table_id_t table_id_;
        std::vector<col_id_t> columns_;
        std::vector<std::unique_ptr<BoundExpression>> filters_;
    };
}

//
// Created by huan.yang on 2026-05-09.
//

#pragma once
#include <memory>

#include "logical_operator.h"
#include "binder/expression/bound_expression.h"

namespace chickenDB {
    class LogicalFilter : public LogicalOperator {
    public:
        explicit LogicalFilter() : LogicalOperator(LogicalOperatorType::FILTER) {
        }

        ~LogicalFilter() override = default;

        std::unique_ptr<BoundExpression> condition_;
    };
}

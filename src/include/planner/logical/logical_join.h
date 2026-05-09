//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <vector>

#include "logical_operator.h"
#include "binder/expression/bound_expression.h"
#include "common/join_type.h"

namespace chickenDB {
    class LogicalJoin : public LogicalOperator {
    public:
        explicit LogicalJoin(JoinType type) : LogicalOperator(LogicalOperatorType::JOIN), type_(type) {
        }

        ~LogicalJoin() override = default;

        JoinType type_;
        std::unique_ptr<LogicalOperator> left_;
        std::unique_ptr<LogicalOperator> right_;
        std::vector<std::unique_ptr<BoundExpression> > conditions_;
    };
}

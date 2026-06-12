//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <vector>

#include "logical_operator.h"
#include "binder/expression/bound_expression.h"
#include "common/join_type.h"
#include "common/types.h"

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

        // 等值连接键（左/右各列 col_id，按位对应）。children_[0]=左 children_[1]=右。
        std::vector<col_id_t> left_keys_;
        std::vector<col_id_t> right_keys_;
    };
}

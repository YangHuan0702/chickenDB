//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include "bound_expression.h"
#include "common/enum/binary_opt_type.h"
#include "sql/SQLStatement.h"

namespace chickenDB {
    class BoundBinaryExpression : public BoundExpression {
    public:
        explicit BoundBinaryExpression(BinaryOpExpressionType type) : BoundExpression(BinderExpressionType::BINARY_OP),
                                                                      type_(type) {
        }

        ~BoundBinaryExpression() override = default;

        BinaryOpExpressionType type_;
        std::unique_ptr<BoundExpression> left_;
        std::unique_ptr<BoundExpression> right_;
    };
}

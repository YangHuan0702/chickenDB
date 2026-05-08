//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <memory>

#include "bound_expression.h"
#include "common/enum/unary_op_type.h"

namespace chickenDB {
    class BoundUnaryExpression : public BoundExpression {
    public:
        explicit BoundUnaryExpression(UnaryOpType type) : BoundExpression(BinderExpressionType::UNARY_OP), type_(type) {
        }

        ~BoundUnaryExpression() override = default;

        UnaryOpType type_;
        std::unique_ptr<BoundExpression> left_;
    };
}

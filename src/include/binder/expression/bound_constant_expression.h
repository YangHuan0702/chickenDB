//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include "bound_expression.h"
#include "common/value.h"

namespace chickenDB {
    class BoundConstantExpression : public BoundExpression {
    public:
        explicit BoundConstantExpression(Value value) : BoundExpression(BinderExpressionType::CONSTANT), val_(value) {
        }

        ~BoundConstantExpression() override = default;

        Value val_;
    };
}

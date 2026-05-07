//
// Created by 杨欢 on 2026/5/6.
//
#pragma once
#include "expression.h"
#include "common/value.h"

namespace chickenDB {
    class ConstantExpression : public ParserExpression {
    public:
        explicit ConstantExpression(const Value &val) : ParserExpression(ParserExpressionType::CONSTANT), val_(val) {
        }
        ~ConstantExpression() override = default;

        Value val_;
    };
}

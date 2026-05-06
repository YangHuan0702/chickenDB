//
// Created by 杨欢 on 2026/5/6.
//
#pragma once
#include <memory>

#include "expression.h"

namespace chickenDB {

    class BinaryOpExpression:public ParserExpression {
    public:
        BinaryOpExpression() : ParserExpression(ParserExpressionType::BINARY_OP) {}
        ~BinaryOpExpression() override = default;

        std::unique_ptr<ParserExpression> left_;
        std::unique_ptr<ParserExpression> right_;
    };

}

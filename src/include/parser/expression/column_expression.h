//
// Created by 杨欢 on 2026/5/6.
//
#pragma once
#include "expression.h"

namespace chickenDB {

    class BinaryOpExpression : public ParserExpression {
    public:
        explicit BinaryOpExpression() : ParserExpression(ParserExpressionType::BINARY_OP) {}
    };

}

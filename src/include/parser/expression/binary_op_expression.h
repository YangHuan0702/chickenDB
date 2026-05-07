//
// Created by 杨欢 on 2026/5/6.
//
#pragma once
#include <memory>

#include "expression.h"
#include "common/enum/binary_opt_type.h"

namespace chickenDB {
    class BinaryOpExpression : public ParserExpression {
    public:
        BinaryOpExpression(BinaryOpExpressionType type) : ParserExpression(ParserExpressionType::BINARY_OP),
                                                          type_(type) {
        }

        ~BinaryOpExpression() override = default;

        BinaryOpExpressionType type_;
        std::unique_ptr<ParserExpression> left_;
        std::unique_ptr<ParserExpression> right_;
    };
}

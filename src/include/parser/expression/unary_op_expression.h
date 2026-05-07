//
// Created by 杨欢 on 2026/5/6.
//
#pragma once
#include <memory>

#include "expression.h"
#include "common/enum/unary_op_type.h"

namespace chickenDB {
    class UnaryOpExpression : public ParserExpression {
    public:
        explicit UnaryOpExpression(UnaryOpType type) : ParserExpression(ParserExpressionType::UNARY_OP), type_(type) {
        }
        ~UnaryOpExpression() override = default;


        UnaryOpType type_;
        std::unique_ptr<ParserExpression> left_;
    };
}

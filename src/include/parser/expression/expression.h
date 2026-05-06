//
// Created by 杨欢 on 2026/5/6.
//
#pragma once
#include "common/enum/parser_expression_type.h"

namespace chickenDB {

    class ParserExpression {
    public:
        ParserExpression(ParserExpressionType type) : type_(type){}
        virtual ~ParserExpression() = default;

        ParserExpressionType type_;
    };

}

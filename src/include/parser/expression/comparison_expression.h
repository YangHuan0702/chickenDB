//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/parsed_expression.h"

namespace chickenDB {

    class ComparisonExpression : public ParsedExpression {
    public:
        ComparisonExpression(ExpressionType type,std::unique_ptr<ParsedExpression> left, std::unique_ptr<ParsedExpression> right);

        std::unique_ptr<ParsedExpression> left;
        std::unique_ptr<ParsedExpression> right;


        std::string ToString() const override;
        bool Equals(const BaseExpression *other) const override;
        std::unique_ptr<ParsedExpression> Copy() const override;
    };

}

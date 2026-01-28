//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <vector>

#include "parser/parsed_expression.h"

namespace chickenDB {

    class OperatorExpression : public ParsedExpression {
    public:
        OperatorExpression(ExpressionType type, std::unique_ptr<ParsedExpression> left = nullptr, std::unique_ptr<ParsedExpression> right = nullptr);


        std::vector<std::unique_ptr<ParsedExpression>> children;

        std::string ToString() const override;

        bool Equals(const BaseExpression *other) const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };

}

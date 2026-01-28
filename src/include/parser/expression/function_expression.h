//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/parsed_expression.h"
#include <vector>

namespace chickenDB {
    class FunctionExpression : public ParsedExpression {
    public:
        FunctionExpression(std::string schema_name,std::string function_name, std::vector<std::unique_ptr<ParsedExpression>> &children);
        FunctionExpression(std::string function_name,std::vector<std::unique_ptr<ParsedExpression>> &children);

        std::string schema;
        std::string function_name;
        std::vector<std::unique_ptr<ParsedExpression>> children;

        uint64_t Hash() const override;

        std::string ToString() const override;

        bool Equals(const BaseExpression *other) const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

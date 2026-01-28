//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/parsed_expression.h"
#include <vector>

#include "column_expression.h"

namespace chickenDB {
    class FunctionExpression : public ParsedExpression {
    public:
        FunctionExpression(std::string schema_name,std::string function_name, std::vector<std::unique_ptr<ParsedExpression>> &children);
        FunctionExpression(std::string function_name,std::vector<std::unique_ptr<ParsedExpression>> &children);

        std::string schema;
        std::string function_name;
        std::vector<std::unique_ptr<ParsedExpression>> children;

        std::string GetName() const override {
            return function_name;
        }

        bool IsAggregate() const override {return false;}

        bool IsScalar() const override {return false;}

        bool IsWindow() const override {return false;}

        bool HasSubQuery() const override {return false;}

        bool HasParameter() const override {return false;}

        uint64_t Hash() const override { return 0;}

        std::string ToString() const override;

        bool Equals(ColumnRefExpression *other) const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

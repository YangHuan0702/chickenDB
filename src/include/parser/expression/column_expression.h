//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/parsed_expression.h"

namespace chickenDB {
    class ColumnRefExpression : public ParsedExpression {
    public:
        ColumnRefExpression(std::string column_name, std::string table_name);
        ColumnRefExpression(std::string column_name);

        std::string column_name;
        std::string table_name;

        bool IsScalar() const override {
            return false;
        }

        std::string GetName() const override;
        std::string ToString() const override;
        bool Equals(const BaseExpression *other) const override;
        uint64_t Hash() const override;
        std::unique_ptr<ParsedExpression> Copy() const override;


    };
}

//
// Created by huan.yang on 2026-01-28.
//
#include "parser/expression/column_expression.h"


namespace chickenDB {

    ColumnRefExpression::ColumnRefExpression(std::string column_name, std::string table_name)
    : ParsedExpression(ExpressionType::COLUMN_REF, ExpressionClass::COLUMN_REF), column_name(column_name), table_name(table_name) {
    }

    ColumnRefExpression::ColumnRefExpression(std::string column_name) : ColumnRefExpression(column_name,std::string()) {
    }

    std::unique_ptr<ParsedExpression> ColumnRefExpression::Copy() const {
        auto copy = std::make_unique<ColumnRefExpression>(column_name, table_name);
        copy->CopyProperties(*this);
        return std::move(copy);
    }


    uint64_t ColumnRefExpression::Hash() const {
        return 0;
    }

    std::string ColumnRefExpression::GetName() const {
        return !alias.empty() ? alias : column_name;
    }

    std::string ColumnRefExpression::ToString() const {
        if (table_name.empty()) {
            return column_name;
        } else {
            return table_name + "." + column_name;
        }
    }

    bool ColumnRefExpression::Equals(const BaseExpression *other_) const {
        if (!BaseExpression::Equals(other_)) {
            return false;
        }
        auto other = (ColumnRefExpression *)other_;
        return column_name == other->column_name && table_name == other->table_name;
    }


}

//
// Created by 杨欢 on 2026/5/6.
//
#pragma once
#include<string>
#include <utility>
#include "expression.h"

namespace chickenDB {
    class ColumnRefExpression : public ParserExpression {
    public:
        explicit ColumnRefExpression(std::string table_name, std::string column_name) : ParserExpression(
                ParserExpressionType::COLUMN),
            table_name_(std::move(table_name)), column_name_(std::move(column_name)) {
        }

        ~ColumnRefExpression() override = default;

        std::string table_name_;
        std::string column_name_;
    };
}

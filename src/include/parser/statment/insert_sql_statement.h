//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <string>
#include <vector>

#include "sql_statement.h"
#include "../../common/value.h"

namespace chickenDB {
    class InsertStatement : public SQLStatement {
    public:
        InsertStatement(const std::string &table_name) : SQLStatement(StatementType::INSERT), table_name_(table_name) {
        }

        ~InsertStatement() override = default;

        auto AddColumn(const std::string &column_name, std::unique_ptr<ParserExpression> value) -> void {
            columns_.push_back(column_name);
            values_.push_back(std::move(value));
        }

        std::string table_name_;

        std::vector<std::string> columns_;

        std::vector<std::unique_ptr<ParserExpression>> values_{};
    };
}

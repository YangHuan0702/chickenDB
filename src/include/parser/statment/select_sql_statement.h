//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <string>
#include <vector>
#include "sql_statement.h"

namespace chickenDB {
    class SelectStatement : public SQLStatement {
    public:
        SelectStatement(const std::string &table)  : SQLStatement(StatementType::SELECT), table_(table) {
        }
        ~SelectStatement() override = default;

        auto AddColumn(const std::string &column_name) -> void {
            this->columns_.push_back(std::move(column_name));
        }

        std::string table_;

        std::vector<std::string> columns_;
    };
}

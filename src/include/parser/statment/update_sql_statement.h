//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <string>
#include <vector>

#include "sql_statement.h"
#include "common/types/value.h"

namespace chickenDB {
    class UpdateStatement : public SQLStatement {
    public:
        explicit UpdateStatement(const std::string &table_name) : SQLStatement(StatementType::UPDATE),
                                                                  table_name_(table_name) {
        }

        ~UpdateStatement() override = default;


        auto AddColumn(const std::string &column_name, Value value) -> void {
            this->columns_.push_back(column_name);
            this->values_.push_back(std::move(value));
        }

        std::vector<std::string> columns_;
        std::vector<Value> values_;
        std::string table_name_;
    };
}

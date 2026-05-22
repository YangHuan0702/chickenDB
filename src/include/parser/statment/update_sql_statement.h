//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "sql_statement.h"
#include "../../common/value.h"

namespace chickenDB {
    class UpdateStatement : public SQLStatement {
    public:
        explicit UpdateStatement(std::string table_name) : SQLStatement(StatementType::UPDATE),
                                                                  table_name_(std::move(table_name)) {
        }

        ~UpdateStatement() override = default;

        std::vector<std::string> columns_;
        std::vector<Value> values_{};
        std::string table_name_;
        std::unique_ptr<ParserExpression> where_;
    };
}

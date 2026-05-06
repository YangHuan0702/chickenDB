//
// Created by huan.yang on 2026-04-30.
//

#pragma once
#include <string>
#include <utility>
#include "sql_statement.h"

namespace chickenDB {
    class DeleteStatement : public SQLStatement {
    public:
        explicit DeleteStatement(std::string table_name) : SQLStatement(StatementType::DELETE),
                                                                  table_name_(std::move(table_name)) {
        }

        ~DeleteStatement() override = default;

        std::unique_ptr<ParserExpression> where_{};
        std::string table_name_;
    };
}

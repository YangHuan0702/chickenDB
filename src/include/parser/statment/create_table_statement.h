//
// Created by huan.yang on 2026-04-30.
//

#pragma once
#include <string>
#include <vector>

#include "sql_statement.h"

namespace chickenDB {
    class CreateTableStatement : public SQLStatement {
    public:
        explicit CreateTableStatement(const std::string &table_name) : SQLStatement(StatementType::CREATE),
                                                                       table_name_(table_name) {
        }

        ~CreateTableStatement() override = default;

        std::string table_name_;
    };
}

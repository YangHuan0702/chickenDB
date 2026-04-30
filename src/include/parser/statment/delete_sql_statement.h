//
// Created by huan.yang on 2026-04-30.
//

#pragma once
#include <string>
#include "sql_statement.h"

namespace chickenDB {
    class DeleteStatement : public SQLStatement {
    public:
        explicit DeleteStatement(const std::string &table_name) : SQLStatement(StatementType::DELETE),
                                                                  table_name_(table_name) {
        }

        ~DeleteStatement() override = default;




        std::string table_name_;
    };
}

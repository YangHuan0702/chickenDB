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

        std::string table_;

        std::vector<std::unique_ptr<ParserExpression>> columns_;

        std::unique_ptr<ParserExpression> where_;

        std::vector<std::unique_ptr<ParserExpression>> group_;
        std::unique_ptr<ParserExpression> having_;

        std::vector<std::unique_ptr<ParserExpression>> order_;

    };
}

//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "select_statement.h"
#include "../../common/constants/constants.h"
#include "parser/parsed_expression.h"
#include "parser/sql_statement.h"

namespace chickenDB {

    class InsertStatement : public SQLStatement {
    public:
        InsertStatement() : SQLStatement(StatementType::INSERT), schema(DEFAULT_SCHEMA) {}

        std::unique_ptr<SelectStatement> select_statement;
        std::vector<std::vector<std::unique_ptr<ParsedExpression>>> values;
        std::vector<std::string> columns;
        std::string table;
        std::string schema;
    };

}


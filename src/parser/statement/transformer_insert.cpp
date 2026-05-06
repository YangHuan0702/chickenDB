//
// Created by huan.yang on 2026-04-30.
//
#include "parser/transformer.h"
#include "parser/statment/insert_sql_statement.h"
#include "sql/InsertStatement.h"

using namespace chickenDB;

auto Transformer::TransformerInsertStatement(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto insert_statement = dynamic_cast<hsql::InsertStatement*>(statement);

    auto res = std::make_unique<chickenDB::InsertStatement>(insert_statement->tableName);

    size_t insert_column_size = insert_statement->columns->size();
    for (size_t i = 0; i < insert_column_size; i++) {
        res->AddColumn(insert_statement->columns->at(i),TransformerExpression(insert_statement->values->at(i)));
    }
    return res;
}

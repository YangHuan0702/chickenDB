//
// Created by huan.yang on 2026-04-30.
//

#include "parser/transformer.h"
#include "parser/statment/delete_sql_statement.h"
#include "sql/DeleteStatement.h"

using namespace chickenDB;


auto Transformer::TransformerDeleteStatement(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto delete_statement = dynamic_cast<hsql::DeleteStatement*>(statement);
    current_table_name_ = delete_statement->tableName;

    auto res = std::make_unique<DeleteStatement>(delete_statement->tableName);

    auto expr = TransformerExpression(delete_statement->expr);

    res->where_ = std::move(expr);
    return res;
}

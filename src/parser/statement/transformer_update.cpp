//
// Created by huan.yang on 2026-04-30.
//
#include "parser/transformer.h"
#include "parser/statment/update_sql_statement.h"
#include "sql/UpdateStatement.h"

using namespace chickenDB;


auto Transformer::TransformerUpdateStatement(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto update_statement = dynamic_cast<hsql::UpdateStatement*>(statement);

    auto r = std::make_unique<UpdateStatement>(update_statement->table->name);

    for (auto stat : *update_statement->updates) {
        r->columns_.push_back(stat->column);
        r->values_.push_back(TransformerExpression(stat->value));
    }

    r.where_ = TransformerExpression(update_statement->where);
    return r;
}

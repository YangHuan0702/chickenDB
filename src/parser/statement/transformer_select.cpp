//
// Created by huan.yang on 2026-04-30.
//
#include "parser/transformer.h"
#include "parser/statment/select_sql_statement.h"
#include "sql/SelectStatement.h"

using namespace chickenDB;

auto Transformer::TransformerSelectStatement(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto select_statement = dynamic_cast<hsql::SelectStatement*>(statement);

    auto r = std::make_unique<SelectStatement>(select_statement->fromTable->name);

    if (!select_statement->selectList->empty()) {
        for (auto sel : *select_statement->selectList) {
            r->columns_.push_back(TransformerExpression(sel));
        }
    }

    if (select_statement->whereClause != nullptr) {
        r->where_ = TransformerExpression(select_statement->whereClause);
    }
    return r;
}





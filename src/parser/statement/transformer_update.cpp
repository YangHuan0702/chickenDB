//
// Created by huan.yang on 2026-04-30.
//
#include "parser/transformer.h"
#include "parser/expression/constant_expression.h"
#include "parser/statment/update_sql_statement.h"
#include "sql/Table.h"
#include "sql/UpdateStatement.h"

using namespace chickenDB;

static auto TransformConstantValue(Transformer &transformer, hsql::Expr *expr) -> Value {
    auto expression = transformer.TransformerExpression(expr);
    auto *constant_expression = dynamic_cast<ConstantExpression *>(expression.get());
    if (constant_expression == nullptr) {
        throw std::runtime_error("[Parser] update value must be constant");
    }
    return constant_expression->val_;
}

auto Transformer::TransformerUpdateStatement(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto update_statement = dynamic_cast<hsql::UpdateStatement*>(statement);
    current_table_name_ = update_statement->table->name;

    auto r = std::make_unique<UpdateStatement>(update_statement->table->name);

    for (auto stat : *update_statement->updates) {
        r->columns_.push_back(stat->column);
        r->values_.push_back(TransformConstantValue(*this, stat->value));
    }

    r->where_ = TransformerExpression(update_statement->where);
    return r;
}

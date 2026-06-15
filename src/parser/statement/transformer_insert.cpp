//
// Created by huan.yang on 2026-04-30.
//
#include "parser/transformer.h"
#include "parser/expression/constant_expression.h"
#include "parser/statment/insert_sql_statement.h"
#include "sql/InsertStatement.h"

using namespace chickenDB;

static auto TransformConstantValue(Transformer &transformer, hsql::Expr *expr) -> Value {
    auto expression = transformer.TransformerExpression(expr);
    auto *constant_expression = dynamic_cast<ConstantExpression *>(expression.get());
    if (constant_expression == nullptr) {
        throw std::runtime_error("[Parser] insert value must be constant");
    }
    return constant_expression->val_;
}

auto Transformer::TransformerInsertStatement(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto insert_statement = dynamic_cast<hsql::InsertStatement*>(statement);

    current_table_name_ = insert_statement->tableName;
    auto res = std::make_unique<chickenDB::InsertStatement>(insert_statement->tableName);

    size_t insert_column_size = insert_statement->columns->size();
    for (size_t i = 0; i < insert_column_size; i++) {
        res->AddColumn(insert_statement->columns->at(i), TransformConstantValue(*this, insert_statement->values->at(i)));
    }
    return res;
}

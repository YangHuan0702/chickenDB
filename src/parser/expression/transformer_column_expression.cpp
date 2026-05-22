//
// Created by huan.yang on 2026-05-07.
//
#include "parser/transformer.h"
#include "parser/expression/binary_op_expression.h"
#include "parser/expression/column_expression.h"

using namespace chickenDB;

auto Transformer::TransformerColumnRef(hsql::Expr *expr) -> std::unique_ptr<ParserExpression> {
    return std::make_unique<ColumnRefExpression>(expr->table == nullptr ? "" : expr->table,
                                                expr->name == nullptr ? "" : expr->name);
}

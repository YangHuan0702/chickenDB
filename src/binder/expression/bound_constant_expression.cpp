//
// Created by huan.yang on 2026-05-08.
//
#include "binder/expression/bound_constant_expression.h"

#include "binder/binder.h"
#include "parser/expression/constant_expression.h"

using namespace chickenDB;

auto Binder::BoundConstantExpression(
    std::unique_ptr<ParserExpression> expr) -> std::unique_ptr<class chickenDB::BoundExpression> {
    ChickenException::AssertCondition(expr->type_ == ParserExpressionType::CONSTANT,
                                      "[Binder] bound expression type not is constant.");

    auto parser_column_expression = dynamic_cast<ConstantExpression *>(expr.release());

    return std::make_unique<chickenDB::BoundConstantExpression>(parser_column_expression->val_);
}

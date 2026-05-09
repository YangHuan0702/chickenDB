//
// Created by huan.yang on 2026-05-08.
//
#include "binder/expression/bound_unary_expression.h"

#include "binder/binder.h"
#include "parser/expression/unary_op_expression.h"

using namespace chickenDB;


auto Binder::BoundUnaryExpression(
    std::unique_ptr<ParserExpression> expr) -> std::unique_ptr<class chickenDB::BoundExpression> {
    ChickenException::AssertCondition(expr->type_ == ParserExpressionType::UNARY_OP,
                                      "[Binder] bound expression type not is unary.");

    auto parser_unary_expression = dynamic_cast<UnaryOpExpression *>(expr.release());

    auto bound_unary_expression = std::make_unique<chickenDB::BoundUnaryExpression>(parser_unary_expression->type_);

    bound_unary_expression->left_ = BoundExpression(std::move(parser_unary_expression->left_));
    return bound_unary_expression;
}

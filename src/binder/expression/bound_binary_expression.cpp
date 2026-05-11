//
// Created by huan.yang on 2026-05-08.
//
#include "binder/expression/bound_binary_expression.h"

#include "binder/binder.h"
#include "parser/expression/binary_op_expression.h"

using namespace chickenDB;

auto Binder::BoundBinaryExpression(std::unique_ptr<ParserExpression> expr) -> std::unique_ptr<class BoundExpression> {
    ChickenException::AssertCondition(expr->type_ == ParserExpressionType::BINARY_OP, "[Binder] bound expression type not is binary.");

    auto parser_binary_expression = dynamic_cast<BinaryOpExpression *>(expr.get());

    auto bound_binary_expression = std::make_unique<chickenDB::BoundBinaryExpression>(parser_binary_expression->type_);

    bound_binary_expression->left_ = BoundExpression(std::move(parser_binary_expression->left_));
    bound_binary_expression->right_ = BoundExpression(std::move(parser_binary_expression->right_));
    return bound_binary_expression;
}

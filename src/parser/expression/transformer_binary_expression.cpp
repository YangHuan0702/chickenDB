//
// Created by huan.yang on 2026-05-07.
//
#include "parser/transformer.h"
#include "parser/expression/binary_op_expression.h"

using namespace chickenDB;

static auto transformerExprType(hsql::Expr *expr) -> BinaryOpExpressionType {
    switch (expr->opType) {
        case hsql::kOpPlus: return BinaryOpExpressionType::ADD;
        case hsql::kOpMinus: return BinaryOpExpressionType::SUB;
        case hsql::kOpAsterisk: return BinaryOpExpressionType::MUL;
        case hsql::kOpSlash: return BinaryOpExpressionType::DRI;

        case hsql::kOpEquals: return BinaryOpExpressionType::EQ;
        case hsql::kOpNotEquals: return BinaryOpExpressionType::NE;

        case hsql::kOpLess: return BinaryOpExpressionType::LT;
        case hsql::kOpLessEq: return BinaryOpExpressionType::LTE;
        case hsql::kOpGreater: return BinaryOpExpressionType::GT;
        case hsql::kOpGreaterEq: return BinaryOpExpressionType::GTE;

        case hsql::kOpLike: return BinaryOpExpressionType::LIKE;
        case hsql::kOpNotLike: return BinaryOpExpressionType::NOT_LIKE;
        case hsql::kOpILike: return BinaryOpExpressionType::ILIKE;

        case hsql::kOpAnd: return BinaryOpExpressionType::AND;
        case hsql::kOpOr: return BinaryOpExpressionType::OR;
        case hsql::kOpIn: return BinaryOpExpressionType::IN;
        default: throw std::runtime_error("[Parser] Invalid expression binary type");
    }
}


auto Transformer::TransformerBinaryOperator(hsql::Expr *expr) -> std::unique_ptr<ParserExpression> {
    auto transformer_expr_type = transformerExprType(expr);

    auto res = std::make_unique<BinaryOpExpression>(transformer_expr_type);
    res->left_ = TransformerExpression(expr->expr);
    res->right_ = TransformerExpression(expr->expr2);
    return res;
}

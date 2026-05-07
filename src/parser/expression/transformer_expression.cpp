//
// Created by huan.yang on 2026-05-07.
//

#include "parser/transformer.h"

using namespace chickenDB;

auto Transformer::TransformerOperatorExpression(hsql::Expr *expr) -> std::unique_ptr<ParserExpression> {
    switch (expr->opType) {
        case hsql::kOpLess:
        case hsql::kOpLessEq:
        case hsql::kOpGreater:
        case hsql::kOpGreaterEq:
        case hsql::kOpEquals:
        case hsql::kOpNotEquals:
        case hsql::kOpAnd:
        case hsql::kOpOr:
        case hsql::kOpPlus:
        case hsql::kOpMinus:
        case hsql::kOpAsterisk:
        case hsql::kOpSlash:
            return TransformerBinaryOperator(expr);
        case hsql::kOpIsNull:
        case hsql::kOpNot:
        case hsql::kOpExists:
            return TransformerUnaryOperator(expr);
        default: throw std::invalid_argument("[Parser] Invalid operator expression");
    }
}


auto Transformer::TransformerExpression(hsql::Expr *expr) -> std::unique_ptr<ParserExpression> {
    switch (expr->type) {
        case hsql::kExprOperator:
            return TransformerOperatorExpression(expr);
        case hsql::kExprStar:
            return TransformerStar(expr);
        case hsql::kExprColumnRef:
            return TransformerColumnRef(expr);
        case hsql::kExprLiteralFloat:
        case hsql::kExprLiteralString:
        case hsql::kExprLiteralInt:
        case hsql::kExprLiteralNull:
            return TransformerConstant(expr);
        default: throw std::runtime_error("[Parser] Invalid expression type");
    }
}

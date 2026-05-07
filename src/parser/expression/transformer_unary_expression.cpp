//
// Created by huan.yang on 2026-05-07.
//
#include "common/enum/unary_op_type.h"
#include "parser/transformer.h"
#include "parser/expression/unary_op_expression.h"

using namespace chickenDB;

static auto transformerUnaryOperatorType(hsql::Expr *expr) -> UnaryOpType {
    switch (expr->opType) {
        case hsql::kOpNot: return UnaryOpType::IS_NULL;
        case hsql::kOpIsNull: return UnaryOpType::NON_NULL;
        default: throw std::runtime_error("unary operator type not supported");
    }
}


auto Transformer::TransformerUnaryOperator(hsql::Expr *expr) -> std::unique_ptr<ParserExpression> {
    auto unary_type = transformerUnaryOperatorType(expr);
    auto res = std::make_unique<UnaryOpExpression>(unary_type);

    res->left_ = TransformerExpression(expr->expr);
    return res;
}

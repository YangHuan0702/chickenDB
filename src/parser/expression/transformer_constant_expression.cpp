//
// Created by huan.yang on 2026-05-07.
//
#include "parser/transformer.h"
#include "parser/expression/constant_expression.h"

using namespace chickenDB;


auto Transformer::TransformerConstant(hsql::Expr *expr) -> std::unique_ptr<ParserExpression> {
    std::variant<std::monostate,int, char, std::string, float, double, int64_t> value;
    switch (expr->type) {
        case hsql::kExprLiteralFloat:
            value = expr->fval;
            break;
        case hsql::kExprLiteralString:
            value = expr->name;
            break;
        case hsql::kExprLiteralInt:
            value = expr->ival;
            break;
        case hsql::kExprLiteralNull:
            value = std::monostate{};
            break;
        default: throw std::runtime_error("[Parser] Invalid constant type");
    }
    return std::make_unique<ConstantExpression>(Value(value));
}
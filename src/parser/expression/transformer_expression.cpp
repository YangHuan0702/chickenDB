//
// Created by huan.yang on 2026-05-07.
//

#include "parser/transformer.h"
#include "parser/expression/column_expression.h"

#include <cctype>

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
        case hsql::kOpLike:
        case hsql::kOpNotLike:
        case hsql::kOpILike:
        case hsql::kOpIn:
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
        case hsql::kExprFunctionRef: {
            // 聚合函数 SUM(col)/COUNT(col)/MIN/MAX/AVG：取第一个参数列 + 函数名。
            std::string col_name;
            if (expr->exprList != nullptr && !expr->exprList->empty()) {
                hsql::Expr *arg = expr->exprList->at(0);
                if (arg->name != nullptr) col_name = arg->name;
            }
            auto col = std::make_unique<ColumnRefExpression>("", col_name);
            col->is_aggregate_ = true;
            if (expr->name != nullptr) {
                std::string fn = expr->name;
                for (auto &ch : fn) ch = static_cast<char>(::toupper(ch));
                col->agg_func_ = fn;
            }
            return col;
        }
        case hsql::kExprLiteralFloat:
        case hsql::kExprLiteralString:
        case hsql::kExprLiteralInt:
        case hsql::kExprLiteralNull:
            return TransformerConstant(expr);
        default: throw std::runtime_error("[Parser] Invalid expression type");
    }
}

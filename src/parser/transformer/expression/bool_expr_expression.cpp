//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"
#include "parser/expression/conjunction_expression.h"
#include "parser/expression/operator_expression.h"
using namespace chickenDB;

std::unique_ptr<ParsedExpression> Transformer::TransformBoolExpr(duckdb_libpgquery::PGBoolExpr *root) {
    std::unique_ptr<ParsedExpression> result;
    for (auto node = root->args->head; node != nullptr; node = node->next) {
        auto next = TransformExpression(reinterpret_cast<duckdb_libpgquery::PGNode *>(node->data.ptr_value));

        switch (root->boolop) {
            case duckdb_libpgquery::PG_AND_EXPR: {
                if (!result) {
                    result = std::move(next);
                } else {
                    result = std::make_unique<ConjunctionExpression>(ExpressionType::CONJUNCTION_AND, std::move(result), std::move(next));
                }
                break;
            }
            case duckdb_libpgquery::PG_OR_EXPR: {
                if (!result) {
                    result = std::move(next);
                } else {
                    result = std::make_unique<ConjunctionExpression>(ExpressionType::CONJUNCTION_OR, std::move(result), std::move(next));
                }
                break;
            }
            case duckdb_libpgquery::PG_NOT_EXPR: {
                if (next->type == ExpressionType::COMPARE_IN) {
                    // convert COMPARE_IN to COMPARE_NOT_IN
                    next->type = ExpressionType::COMPARE_NOT_IN;
                    result = std::move(next);
                } else {
                    result = std::make_unique<OperatorExpression>(ExpressionType::OPERATOR_NOT, std::move(next));
                }
                break;
            }
        }
    }
    return result;
}

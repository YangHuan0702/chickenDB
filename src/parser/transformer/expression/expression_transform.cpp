//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"
#include "parser/expression/default_expression.h"

namespace chickenDB {

    bool Transformer::TransformerExpressionList(duckdb_libpgquery::PGList *list, std::vector<std::unique_ptr<ParsedExpression> > &result) {
        if (!list) {
            return false;
        }
        for (auto node = list->head; node != nullptr; node = node->next) {
            auto target = reinterpret_cast<duckdb_libpgquery::PGNode *>(node->data.ptr_value);
            if (!target) {
                return false;
            }
            auto expr = TransformExpression(target);
            if (!expr) {
                return false;
            }
            result.push_back(std::move(expr));
        }
        return true;
    }


    std::unique_ptr<ParsedExpression> Transformer::TransformExpression(duckdb_libpgquery::PGNode *node) {
        if (!node) {
            return nullptr;
        }
        switch (node->type) {
            case duckdb_libpgquery::T_PGColumnRef:
                return TransformColumnRef(reinterpret_cast<duckdb_libpgquery::PGColumnRef*>(node));
            case duckdb_libpgquery::T_PGAConst:
                return TransformConstant(reinterpret_cast<duckdb_libpgquery::PGAConst*>(node));
            case duckdb_libpgquery::T_PGAExpr:
                return TransformAExpr(reinterpret_cast<duckdb_libpgquery::PGAExpr*>(node));
            case duckdb_libpgquery::T_PGFuncCall:
                return TransformFuncCall(reinterpret_cast<duckdb_libpgquery::PGFuncCall*>(node));
            case duckdb_libpgquery::T_PGBoolExpr:
                return TransformBoolExpr(reinterpret_cast<duckdb_libpgquery::PGBoolExpr*>(node));
            // case duckdb_libpgquery::T_PGTypeCast:
            //     return TransformTypeCast(reinterpret_cast<duckdb_libpgquery::PGTypeCast*>(node));
            // case duckdb_libpgquery::T_PGCaseExpr:
            //     return TransformCase(reinterpret_cast<duckdb_libpgquery::PGCaseExpr*>(node));
            // case duckdb_libpgquery::T_PGSubLink:
            //     return TransformSubquery(reinterpret_cast<duckdb_libpgquery::PGSubLink *>(node));
            // case duckdb_libpgquery::T_PGCoalesceExpr:
            //     return TransformCoalesce(reinterpret_cast<duckdb_libpgquery::PGAExpr *>(node));
            // case duckdb_libpgquery::T_PGNullTest:
            //     return TransformNullTest(reinterpret_cast<duckdb_libpgquery::PGNullTest *>(node));
            // case duckdb_libpgquery::T_PGResTarget:
            //     return TransformResTarget(reinterpret_cast<duckdb_libpgquery::PGResTarget *>(node));
            // case duckdb_libpgquery::T_PGParamRef:
            //     return TransformParamRef(reinterpret_cast<duckdb_libpgquery::PGParamRef *>(node));
            case duckdb_libpgquery::T_PGSetToDefault:
                return std::make_unique<DefaultExpression>();
            default:
                throw std::runtime_error("Expr of type  " + std::to_string(node->type) + " not implemented\n");
        }
    }
}

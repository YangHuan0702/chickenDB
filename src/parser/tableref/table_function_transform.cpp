//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"
#include "parser/tableref/table_function.h"

namespace chickenDB {

    std::unique_ptr<TableRef> Transformer::TransformRangeFunction(duckdb_libpgquery::PGRangeFunction *root) {
        if (root->lateral) {
            throw std::runtime_error("LATERAL not implemented");
        }
        if (root->ordinality) {
            throw std::runtime_error("WITH ORDINALITY not implemented");
        }
        if (root->is_rowsfrom) {
            throw std::runtime_error("ROWS FROM() not implemented");
        }
        if (root->functions->length != 1) {
            throw std::runtime_error("Need exactly one function");
        }


        auto function_sublist = (duckdb_libpgquery::PGList *)root->functions->head->data.ptr_value;
        assert(function_sublist->length == 2);
        auto call_tree = (duckdb_libpgquery::PGNode *)function_sublist->head->data.ptr_value;
        auto coldef = function_sublist->head->next->data.ptr_value;

        assert(call_tree->type == duckdb_libpgquery::T_PGFuncCall);
        if (coldef) {
            throw std::runtime_error("Explicit column definition not supported yet");
        }
        // transform the function call
        auto result = std::make_unique<TableFunction>();
        result->function = TransformFuncCall((duckdb_libpgquery::PGFuncCall *)call_tree);
        result->alias = TransformAlias(root->alias);
        return std::move(result);
    }


}

//
// Created by huan.yang on 2026-01-27.
//
#include "parser/transformer.h"

namespace chickenDB {
    std::unique_ptr<TableRef> Transformer::TransformerTableRefNode(duckdb_libpgquery::PGNode *n) {
        switch (n->type) {
            case duckdb_libpgquery::T_PGRangeVar:
                return TransformRangeVar(reinterpret_cast<duckdb_libpgquery::PGRangeVar*>(n));
            case duckdb_libpgquery::T_PGJoinExpr:
                return TransformJoin(reinterpret_cast<duckdb_libpgquery::PGJoinExpr*>(n));
            case duckdb_libpgquery::T_PGRangeSubselect:
                return TransformRangeSubselect(reinterpret_cast<duckdb_libpgquery::PGRangeSubselect*>(n));
            case duckdb_libpgquery::T_PGRangeFunction:
                return TransformRangeFunction(reinterpret_cast<duckdb_libpgquery::PGRangeFunction*>(n));
            default:
                throw std::runtime_error("From type " + std::to_string(n->type)  + " is not supported");
        }
    }

}
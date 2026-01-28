//
// Created by huan.yang on 2026-01-27.
//
#include "parser/statement/select_statement.h"

#include "nodes/parsenodes.hpp"
#include "parser/transformer.h"

namespace chickenDB {
    std::unique_ptr<SelectStatement> Transformer::TransformerSelect(duckdb_libpgquery::PGNode *node) {
        auto *stmt = reinterpret_cast<duckdb_libpgquery::PGSelectStmt *>(node);
        auto result = std::make_unique<SelectStatement>();
        if (stmt->windowClause) {

        }
        result->node = TransformerSelectNode(stmt);
        return result;
    }
}
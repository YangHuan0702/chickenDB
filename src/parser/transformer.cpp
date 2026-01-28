//
// Created by huan.yang on 2026-01-27.
//

#include "parser/transformer.h"
#include "glog/logging.h"
#include "postgres_parser.hpp"
#include "nodes/parsenodes.hpp"


namespace chickenDB {
    bool Transformer::TransformerPostgresTree(duckdb_libpgquery::PGList *tree,
                                              std::vector<std::unique_ptr<SQLStatement> > &statements) {
        for (auto cell = tree->head; cell; cell = cell->next) {
            auto stmt = TransformerStatement((duckdb_libpgquery::PGNode *) cell->data.ptr_value);
            if (!stmt) {
                statements.clear();
                return false;
            }
            statements.push_back(std::move(stmt));
        }
        return true;
    }


    std::unique_ptr<SQLStatement> Transformer::TransformerStatement(duckdb_libpgquery::PGNode *node) {
        switch (node->type) {
            case duckdb_libpgquery::T_PGRawStmt:
                return TransformerStatement(reinterpret_cast<duckdb_libpgquery::PGRawStmt*>(node)->stmt);
            case duckdb_libpgquery::T_PGSelectStmt:
                return TransformerSelect(node);
            case duckdb_libpgquery::T_PGInsertStmt:
                return TransformerInsert(node);
            case duckdb_libpgquery::T_PGUpdateStmt:
                return TransformerUpdate(node);
            case duckdb_libpgquery::T_PGDeleteStmt:
                return TransformerDelete(node);
            case duckdb_libpgquery::T_PGCreatedbStmt:
                return TransformerCreateDB(node);
            case duckdb_libpgquery::T_PGAlterTableStmt:
                return TransformerCreateTable(node);
            default:
                throw std::runtime_error("un-implement of the Sql Node");
        }
    }

}

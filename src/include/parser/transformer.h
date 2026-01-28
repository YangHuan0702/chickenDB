//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include <vector>
#include <memory>

#include "table_ref.h"
#include "nodes/parsenodes.hpp"
#include "nodes/pg_list.hpp"
#include "parser/sql_statement.h"
#include "parser/query_node.h"
#include "parser/statement/select_statement.h"

namespace chickenDB {


    class Transformer {
    public:
        bool TransformerPostgresTree(duckdb_libpgquery::PGList *tree,std::vector<std::unique_ptr<SQLStatement>> &statements);

        std::unique_ptr<SQLStatement> TransformerStatement(duckdb_libpgquery::PGNode *node);

    private:
        std::string TransformAlias(duckdb_libpgquery::PGAlias *root);
        bool TransformerOrderBy(duckdb_libpgquery::PGList *order,std::vector<OrderByNode> &result);
        bool TransformerExpressionList(duckdb_libpgquery::PGList *list,std::vector<std::unique_ptr<ParsedExpression>> &result);
        std::unique_ptr<ParsedExpression> TransformExpression(duckdb_libpgquery::PGNode *node);
        std::unique_ptr<TableRef> TransformerTableRefNode(duckdb_libpgquery::PGNode *n);
        std::unique_ptr<TableRef> TransformJoin(duckdb_libpgquery::PGJoinExpr* expr);
        std::unique_ptr<TableRef> TransformRangeSubselect(duckdb_libpgquery::PGRangeSubselect* subselect);
        std::unique_ptr<TableRef> TransformRangeFunction(duckdb_libpgquery::PGRangeFunction* function);
        bool TransformGroupBy(duckdb_libpgquery::PGList *group, std::vector<std::unique_ptr<ParsedExpression>> &result);
        std::unique_ptr<TableRef> TransformerFrom(duckdb_libpgquery::PGList *root);
        void TransformValuesList(duckdb_libpgquery::PGList *list,std::vector<std::vector<std::unique_ptr<ParsedExpression>>> &values);

        std::unique_ptr<TableRef> TransformRangeVar(duckdb_libpgquery::PGRangeVar *node);

        // ==================== row ==================
        std::unique_ptr<SelectStatement> TransformerSelect(duckdb_libpgquery::PGNode *node);
        std::unique_ptr<SQLStatement> TransformerInsert(duckdb_libpgquery::PGNode *node);
        std::unique_ptr<SQLStatement> TransformerUpdate(duckdb_libpgquery::PGNode *node);
        std::unique_ptr<SQLStatement> TransformerDelete(duckdb_libpgquery::PGNode *node);

        // ===================== db ===================
        std::unique_ptr<SQLStatement> TransformerCreateDB(duckdb_libpgquery::PGNode *node);

        // ===================== table =================
        std::unique_ptr<SQLStatement> TransformerCreateTable(duckdb_libpgquery::PGNode *node);



        // ======================= node ==========================
        std::unique_ptr<QueryNode> TransformerSelectNode(duckdb_libpgquery::PGSelectStmt *stmt);

    };


}


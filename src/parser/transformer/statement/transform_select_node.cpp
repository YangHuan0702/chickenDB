//
// Created by huan.yang on 2026-01-27.
//
#include "parser/transformer.h"
#include "parser/query_node/select_node.h"
namespace chickenDB {

    std::unique_ptr<QueryNode> Transformer::TransformerSelectNode(duckdb_libpgquery::PGSelectStmt *stmt) {
        std::unique_ptr<QueryNode> node;
        switch (stmt->op) {
            case duckdb_libpgquery::PG_SETOP_NONE: {
                node = std::make_unique<SelectNode>();
                auto result = (SelectNode*)node.get();
                if (stmt->valuesLists) {
                    TransformValuesList(stmt->valuesLists, result->values);
                    return node;
                }

                result->from_table = TransformerFrom(stmt->fromClause);
                TransformGroupBy(stmt->groupClause, result->groups);
                result->having = TransformExpression(stmt->havingClause);
                if (result->groups.size() == 0 && result->having) {
                    throw std::runtime_error("a Group by clause is required before having");
                }
                result->where_clause = TransformExpression(stmt->whereClause);
                if (!TransformerExpressionList(stmt->targetList,result->select_list)) {
                    throw std::runtime_error("Failed to transform target list");
                }
                break;
            }
            default:
                throw std::runtime_error("what the fuck is stmt->op ?");
        }
        TransformerOrderBy(stmt->sortClause,node->orders);
        if (stmt->limitCount) {
            node->limit = TransformExpression(stmt->limitCount);
        }
        if (stmt->limitOffset) {
            node->offset = TransformExpression(stmt->limitOffset);
        }
        return node;
    }


}

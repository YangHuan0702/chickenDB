//
// Created by huan.yang on 2026-01-27.
//
#include "parser/transformer.h"
#include "parser/tableref/base_tableref.h"
#include "common/enum/join_type.h"
#include "parser/tableref/join_ref.h"
#include "parser/expression/column_expression.h"
#include "parser/expression/comparison_expression.h"
#include "parser/expression/conjunction_expression.h"

namespace chickenDB {
    static std::string getTableName(TableRef *table) {
        switch (table->type) {
            case TableReferenceType::BASE_TABLE:
                if (table->alias.size() > 0) {
                    return table->alias;
                }
                return ((BaseTableRef *) table)->table_name;
            case TableReferenceType::SUBQUERY:
                return table->alias;
            default:
                throw std::runtime_error("Invalid table type");
        }
    }

    std::unique_ptr<TableRef> Transformer::TransformJoin(duckdb_libpgquery::PGJoinExpr *root) {
        auto result = std::make_unique<JoinRef>();
        switch (root->jointype) {
            case duckdb_libpgquery::PG_JOIN_INNER: {
                result->type = JoinType::INNER;
                break;
            }
            case duckdb_libpgquery::PG_JOIN_LEFT: {
                result->type = JoinType::LEFT;
                break;
            }
            case duckdb_libpgquery::PG_JOIN_UNIQUE_OUTER: {
                result->type = JoinType::OUTER;
                break;
            }
            case duckdb_libpgquery::PG_JOIN_RIGHT: {
                result->type = JoinType::RIGHT;
                break;
            }
            case duckdb_libpgquery::PG_JOIN_SEMI: {
                result->type = JoinType::SEMI;
                break;
            }
            default: {
                throw std::runtime_error("Join type  " +std::to_string(root->jointype)+ "  not supported yet...\n");
            }
        }

        result->left = TransformerTableRefNode(root->larg);
        result->right = TransformerTableRefNode(root->rarg);

        if (root->usingClause && root->usingClause->length > 0) {
            std::vector<std::string> using_column_names;
            for (auto node= root->usingClause->head; node ; node = node->next) {
                auto target = reinterpret_cast<duckdb_libpgquery::PGNode *>(node->data.ptr_value);
                auto column_name = std::string(reinterpret_cast<duckdb_libpgquery::PGValue*>(target)->val.str);
                using_column_names.push_back(column_name);
            }


            std::unique_ptr<ParsedExpression> join_condition = nullptr;
            for (auto column_name : using_column_names) {
                auto left_expr = std::make_unique<ColumnRefExpression>(column_name,getTableName(result->left.get()));
                auto right_expr = std::make_unique<ColumnRefExpression>(column_name,getTableName(result->right.get()));
                result->hidden_columns.insert(right_expr->table_name + "." + right_expr->column_name);
                auto comp_expr = std::make_unique<ComparisonExpression>(ExpressionType::COMPARE_EQUAL, std::move(left_expr), std::move(right_expr));
                if (!join_condition) {
                    join_condition = std::move(comp_expr);
                } else {
                    join_condition = std::make_unique<ConjunctionExpression>(ExpressionType::CONJUNCTION_AND,
                                                                    std::move(join_condition), std::move(comp_expr));
                }
            }
            result->condition = std::move(join_condition);
        }
        return std::move(result);
    }
}

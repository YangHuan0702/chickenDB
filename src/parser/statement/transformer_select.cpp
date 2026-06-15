//
// Created by huan.yang on 2026-04-30.
//
#include "common/chicken_execption.h"
#include "parser/transformer.h"
#include "parser/statment/select_sql_statement.h"
#include "sql/SelectStatement.h"

using namespace chickenDB;

auto Transformer::TransformerSelectStatement(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto select_statement = dynamic_cast<hsql::SelectStatement*>(statement);
    hsql::TableRef *from = select_statement->fromTable;
    current_table_name_ = from->name;
    std::unique_ptr<SelectStatement> r;

    if (from != nullptr && from->type == hsql::kTableJoin && from->join != nullptr) {
        // 两表 inner equi-join：左右皆为表名，ON 条件转为表达式。
        hsql::JoinDefinition *jd = from->join;
        ChickenException::AssertCondition(jd->left != nullptr && jd->left->type == hsql::kTableName,
                                          "[Parser] join left must be a table");
        ChickenException::AssertCondition(jd->right != nullptr && jd->right->type == hsql::kTableName,
                                          "[Parser] join right must be a table");
        r = std::make_unique<SelectStatement>(jd->left->name);
        r->has_join_ = true;
        r->join_table_ = jd->right->name;
        if (jd->condition != nullptr) {
            r->join_condition_ = TransformerExpression(jd->condition);
        }
    } else {
        r = std::make_unique<SelectStatement>(from->name);
    }

    if (!select_statement->selectList->empty()) {
        for (auto sel : *select_statement->selectList) {
            r->columns_.push_back(TransformerExpression(sel));
        }
    }

    // where
    if (select_statement->whereClause != nullptr) {
        r->where_ = TransformerExpression(select_statement->whereClause);
    }

    // group by
    if (select_statement->groupBy != nullptr) {
        for (auto group_expr : *select_statement->groupBy->columns) {
            r->group_.push_back(TransformerExpression(group_expr));
        }

        // having
        if (select_statement->groupBy->having != nullptr) {
            r->having_ = TransformerExpression(select_statement->groupBy->having);
        }
    }

    // order by
    if (select_statement->order != nullptr) {
        for (auto order_statement : *select_statement->order) {
            r->order_.push_back(TransformerExpression(order_statement->expr));
            r->order_desc_.push_back(order_statement->type == hsql::kOrderDesc);
        }
    }
    return r;
}





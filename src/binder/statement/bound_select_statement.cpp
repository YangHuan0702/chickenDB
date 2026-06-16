//
// Created by huan.yang on 2026-05-08.
//
#include "binder/statement/bound_select_statement.h"

#include "binder/binder.h"
#include "binder/expression/bound_column_expression.h"
#include "common/chicken_execption.h"
#include "parser/statment/select_sql_statement.h"
using namespace chickenDB;

namespace {
    auto ExpandStar(const std::shared_ptr<Catalog> &catalog, table_id_t table_id,
                    std::vector<std::unique_ptr<BoundExpression>> &columns) -> void {
        const SchemaPage *schema = catalog->GetSchema(table_id);
        ChickenException::AssertCondition(schema != nullptr, "[Binder] unknown schema for select *");
        for (const auto &col : schema->columns_) {
            columns.push_back(std::make_unique<BoundColumnExpression>(table_id, col.col_id));
        }
    }
}

auto Binder::BinderSelectStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement> {
    ChickenException::AssertCondition(statement->type_ == StatementType::SELECT,
                                      "[Binder] target parser statement is not select type.");
    auto parser_select_statement = dynamic_cast<SelectStatement *>(statement.get());

    auto table_catalog_entry = catalog_->GetTable(parser_select_statement->table_);
    ChickenException::AssertCondition(table_catalog_entry != nullptr,
                                      "[Binder] Unknown table " + parser_select_statement->table_);

    auto bound_select_statement = std::make_unique<BoundSelectStatement>(table_catalog_entry->table_id);

    // 设置当前表上下文，供无表限定的列名解析。
    current_table_id_ = table_catalog_entry->table_id;
    has_current_table_ = true;

    // JOIN：登记右表 + 绑定 ON 条件（条件里列均为限定名 t.c，按表名解析）。
    if (parser_select_statement->has_join_) {
        auto join_table = catalog_->GetTable(parser_select_statement->join_table_);
        ChickenException::AssertCondition(join_table != nullptr,
                                          "[Binder] Unknown join table " + parser_select_statement->join_table_);
        bound_select_statement->has_join_ = true;
        bound_select_statement->join_table_id_ = join_table->table_id;
        if (parser_select_statement->join_condition_ != nullptr) {
            bound_select_statement->join_condition_ =
                BoundExpression(std::move(parser_select_statement->join_condition_));
        }
    }

    for (auto &parser_expression: parser_select_statement->columns_) {
        if (parser_expression == nullptr) {
            ExpandStar(catalog_, bound_select_statement->table_id_, bound_select_statement->columns_);
            if (bound_select_statement->has_join_) {
                ExpandStar(catalog_, bound_select_statement->join_table_id_, bound_select_statement->columns_);
            }
            continue;
        }
        bound_select_statement->columns_.push_back(BoundExpression(std::move(parser_expression)));
    }

    if (nullptr != parser_select_statement->where_) {
        bound_select_statement->where_ = BoundExpression(std::move(parser_select_statement->where_));
    }

    if (!parser_select_statement->group_.empty()) {
        for (auto &parser_expression: parser_select_statement->group_) {
            bound_select_statement->group_.push_back(BoundExpression(std::move(parser_expression)));
        }

        if (nullptr != parser_select_statement->having_) {
            bound_select_statement->having_ = BoundExpression(std::move(parser_select_statement->having_));
        }
    }

    if (!parser_select_statement->order_.empty()) {
        for (auto &parser_expression: parser_select_statement->order_) {
            bound_select_statement->order_.push_back(BoundExpression(std::move(parser_expression)));
        }
        bound_select_statement->order_desc_ = parser_select_statement->order_desc_;
    }
    return bound_select_statement;
}

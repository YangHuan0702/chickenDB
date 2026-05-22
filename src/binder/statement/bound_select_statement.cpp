//
// Created by huan.yang on 2026-05-08.
//
#include "binder/statement/bound_select_statement.h"

#include "binder/binder.h"
#include "common/chicken_execption.h"
#include "parser/statment/select_sql_statement.h"
using namespace chickenDB;


auto Binder::BinderSelectStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement> {
    ChickenException::AssertCondition(statement->type_ == StatementType::SELECT,
                                      "[Binder] target parser statement is not select type.");
    auto parser_select_statement = dynamic_cast<SelectStatement *>(statement.get());

    auto table_catalog_entry = catalog_->GetTable(parser_select_statement->table_);
    ChickenException::AssertCondition(table_catalog_entry != nullptr,
                                      "[Binder] Unknown table " + parser_select_statement->table_);

    auto bound_select_statement = std::make_unique<BoundSelectStatement>(table_catalog_entry->table_id);

    for (auto &parser_expression: parser_select_statement->columns_) {
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
    }
    return bound_select_statement;
}

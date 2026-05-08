//
// Created by huan.yang on 2026-05-08.
//
#include "binder/statement/bound_update_statement.h"

#include "binder/binder.h"
#include "parser/statment/update_sql_statement.h"

using namespace chickenDB;

auto Binder::BinderUpdateStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement> {
    ChickenException::AssertCondition(statement->type_ == StatementType::UPDATE,
                                      "[Binder] target parser statement is not update type.");

    auto parser_update_statement = dynamic_cast<UpdateStatement *>(statement.release());

    auto table_catalog_entry = catalog_->GetTable(parser_update_statement->table_name_);
    ChickenException::AssertCondition(table_catalog_entry != nullptr,
                                      "[Binder] Unknown table " + parser_update_statement->table_name_);


    auto bound_update_statement = std::make_unique<BoundUpdateStatement>(table_catalog_entry->table_id);

    std::unordered_map<std::string, ColDef *> column_definitions;
    auto col_defs = catalog_->GetSchema(table_catalog_entry->table_id)->columns_;
    std::transform(col_defs.begin(), col_defs.end(), std::inserter(column_definitions, column_definitions.begin())
                   , [](ColDef &col) {
                       return std::make_pair(col.col_name, &col);
                   }
    );

    for (auto column : parser_update_statement->columns_) {
        bound_update_statement->col_ids_.push_back(column_definitions[column]->col_id);
    }

    for (auto &parser_expression : parser_update_statement->values_) {
        bound_update_statement->values_.push_back(BoundExpression(std::move(parser_expression)));
    }

    bound_update_statement->where_ = BoundExpression(std::move(parser_update_statement->where_));

    return bound_update_statement;
}

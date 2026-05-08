//
// Created by huan.yang on 2026-05-08.
//
#include "binder/statement/bound_delete_statement.h"

#include "binder/binder.h"
#include "parser/statment/delete_sql_statement.h"

using namespace chickenDB;


auto Binder::BinderDeleteStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement> {
    ChickenException::AssertCondition(statement->type_ == StatementType::DELETE,
                                      "[Binder] target parser statement is not delete type.");
    auto parser_delete_statement = dynamic_cast<DeleteStatement*>(statement.release());

    auto table_catalog_entry = catalog_->GetTable(parser_delete_statement->table_name_);
    ChickenException::AssertCondition(table_catalog_entry != nullptr,"[Binder] Unknown table " + parser_delete_statement->table_name_);
    auto bound_delete_statement = std::make_unique<BoundDeleteStatement>(table_catalog_entry->table_id);

    ChickenException::AssertCondition(parser_delete_statement->where_ != nullptr, "Error! delete statement need where expr.");
    bound_delete_statement->where_ = BoundExpression(std::move(parser_delete_statement->where_));

    return bound_delete_statement;
}


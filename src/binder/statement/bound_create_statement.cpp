//
// Created by huan.yang on 2026-05-08.
//
#include "binder/statement/bound_create_statement.h"

#include "binder/binder.h"
#include "binder/statement/bound_select_statement.h"
#include "common/chicken_execption.h"

using namespace chickenDB;


auto Binder::BinderCreateStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement> {
    ChickenException::AssertCondition(statement->type_ == StatementType::CREATE,
                                      "[Binder] target parser statement is not create type.");

    auto *parser_create_statement = dynamic_cast<CreateTableStatement *>(statement.get());

    auto bound_create_statement = std::make_unique<BoundCreateStatement>(parser_create_statement->table_name_);
    bound_create_statement->columns_ = std::move(parser_create_statement->columns_);
    return bound_create_statement;
}

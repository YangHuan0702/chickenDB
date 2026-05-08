//
// Created by huan.yang on 2026-05-08.
//
#include "binder/statement/bound_create_statement.h"

#include "binder/binder.h"
#include "binder/statement/bound_select_statement.h"

using namespace chickenDB;


auto Binder::BinderCreateStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement> {
    ChickenException::AssertCondition(statement->type_ == StatementType::CREATE,
                                      "[Binder] target parser statement is not create type.");

    auto *parser_create_statement = dynamic_cast<CreateTableStatement *>(statement.release());
    auto table_id = catalog_->GetTable(parser_create_statement->table_name_)->table_id;

    auto bound_create_statement = std::make_unique<BoundCreateStatement>(table_id);

    std::unordered_map<std::string, ColDef *> column_definitions;
    auto col_defs = catalog_->GetSchema(table_id)->columns_;
    std::transform(col_defs.begin(), col_defs.end(), std::inserter(column_definitions, column_definitions.begin())
                   , [](ColDef &col) {
                       return std::make_pair(col.col_name, &col);
                   }
    );
    for (const auto &column: parser_create_statement->columns_) {
        bound_create_statement->col_ids_.push_back(column_definitions[column.name_]->col_id);
    }

    return bound_create_statement;
}

//
// Created by huan.yang on 2026-05-08.
//
#include "binder/statement/bound_insert_statement.h"
#include "binder/binder.h"
#include "common/chicken_execption.h"
#include "parser/statment/insert_sql_statement.h"

using namespace chickenDB;

auto Binder::BinderInsertStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement> {
    ChickenException::AssertCondition(statement->type_ == StatementType::INSERT,
                                      "[Binder] target parser insert is not delete type.");
    auto parser_insert_Statement = dynamic_cast<InsertStatement*>(statement.get());

    auto table_catalog_entry = catalog_->GetTable(parser_insert_Statement->table_name_);
    ChickenException::AssertCondition(table_catalog_entry != nullptr,"[Binder] Unknown table " + parser_insert_Statement->table_name_);

    auto bound_insert_statement = std::make_unique<BoundInsertStatement>(table_catalog_entry->table_id);


    std::unordered_map<std::string, ColDef *> column_definitions;
    auto col_defs = catalog_->GetSchema(table_catalog_entry->table_id)->columns_;
    std::transform(col_defs.begin(), col_defs.end(), std::inserter(column_definitions, column_definitions.begin())
                   , [](ColDef &col) {
                       return std::make_pair(col.col_name, &col);
                   }
    );

    for (const auto &column: parser_insert_Statement->columns_) {
        bound_insert_statement->col_ids_.push_back(column_definitions[column]->col_id);
    }

    bound_insert_statement->values_ = std::move(parser_insert_Statement->values_);
    // for (auto &parser_expression : parser_insert_Statement->values_) {
    //     bound_insert_statement->values_.push_back(BoundExpression(std::move(parser_expression)));
    // }
    return bound_insert_statement;
}

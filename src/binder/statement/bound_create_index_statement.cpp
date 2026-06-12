//
// Created by huan.yang on 2026-06-11.
//
#include "binder/statement/bound_create_index_statement.h"

#include <unordered_map>

#include "binder/binder.h"
#include "common/chicken_execption.h"
#include "parser/statment/create_index_statement.h"

using namespace chickenDB;

auto Binder::BinderCreateIndexStatement(std::unique_ptr<SQLStatement> statement)
    -> std::unique_ptr<BoundStatement> {
    ChickenException::AssertCondition(statement->type_ == StatementType::CREATE_INDEX,
                                      "[Binder] statement is not CREATE INDEX type.");
    auto *parser_stmt = dynamic_cast<CreateIndexStatement *>(statement.get());

    auto *table = catalog_->GetTable(parser_stmt->table_name_);
    ChickenException::AssertCondition(table != nullptr,
                                      "[Binder] Unknown table " + parser_stmt->table_name_);

    const SchemaPage *schema = catalog_->GetSchema(table->table_id);
    ChickenException::AssertCondition(schema != nullptr, "[Binder] schema not found");

    // 列名 -> col_id 映射。
    std::unordered_map<std::string, col_id_t> name_to_id;
    for (const auto &col : schema->columns_) {
        name_to_id[col.GetColumnName()] = col.col_id;
    }

    auto bound = std::make_unique<BoundCreateIndexStatement>();
    bound->index_name_ = parser_stmt->index_name_;
    bound->table_id_ = table->table_id;
    bound->unique_ = parser_stmt->unique_;
    for (const auto &col_name : parser_stmt->columns_) {
        auto it = name_to_id.find(col_name);
        ChickenException::AssertCondition(it != name_to_id.end(),
                                          "[Binder] unknown index column " + col_name);
        bound->key_cols_.push_back(it->second);
    }
    return bound;
}

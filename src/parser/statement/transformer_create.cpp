//
// Created by huan.yang on 2026-04-30.
//
#include "common/chicken_execption.h"
#include "parser/transformer.h"
#include "parser/statment/create_table_statement.h"
#include "parser/statment/create_index_statement.h"
#include "sql/CreateStatement.h"

using namespace chickenDB;


static auto ConversionChickenDBColumnType(hsql::ColumnType column_type) -> ColumnType {
    switch (column_type.data_type) {
        case hsql::DataType::VARCHAR: return ColumnType::VARCHAR;
        case hsql::DataType::INT:
        case hsql::DataType::BIGINT:
        case hsql::DataType::SMALLINT:
        case hsql::DataType::LONG:
            return ColumnType::NUMBER;
        case hsql::DataType::DECIMAL:
        case hsql::DataType::FLOAT:
        case hsql::DataType::DOUBLE:
            return ColumnType::DOUBLE;
        default: throw ChickenException("unknown columnType ");
    }
}

auto Transformer::TransformerCreateTable(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto *create_statement = dynamic_cast<hsql::CreateStatement*>(statement);
    auto res = std::make_unique<CreateTableStatement>(create_statement->tableName);

    for (auto column : *create_statement->columns) {
        auto currentType  = ConversionChickenDBColumnType(column->type);
        ColumnDefine column_define;
        column_define.name_ = column->name;
        column_define.type_ = currentType;
        column_define.size_ = column->type.length;
        res->AddColumn(column_define);
    }
    return res;
}

// 按 CreateType 分发：CREATE TABLE / CREATE INDEX。
auto Transformer::TransformerCreate(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto *create_statement = dynamic_cast<hsql::CreateStatement *>(statement);
    switch (create_statement->type) {
        case hsql::kCreateTable: return TransformerCreateTable(statement);
        case hsql::kCreateIndex: return TransformerCreateIndex(statement);
        default: throw ChickenException("[Transformer] unsupported CREATE type");
    }
}

auto Transformer::TransformerCreateIndex(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto *create_statement = dynamic_cast<hsql::CreateStatement *>(statement);
    ChickenException::AssertCondition(create_statement->indexName != nullptr,
                                      "[Transformer] CREATE INDEX missing index name");
    ChickenException::AssertCondition(create_statement->tableName != nullptr,
                                      "[Transformer] CREATE INDEX missing table name");

    auto res = std::make_unique<CreateIndexStatement>(create_statement->indexName,
                                                      create_statement->tableName);
    if (create_statement->indexColumns != nullptr) {
        for (char *col : *create_statement->indexColumns) {
            res->columns_.emplace_back(col);
        }
    }
    return res;
}

//
// Created by huan.yang on 2026-05-08.
//
#include "binder/expression/bound_column_expression.h"

#include "binder/binder.h"
#include "common/chicken_execption.h"
#include "parser/expression/column_expression.h"

using namespace chickenDB;


auto Binder::BoundColumnExpression(std::unique_ptr<ParserExpression> expr) -> std::unique_ptr<class chickenDB::BoundExpression> {
    ChickenException::AssertCondition(expr->type_ == ParserExpressionType::COLUMN, "[Binder] bound expression type not is column.");

    auto parser_column_expression = dynamic_cast<ColumnRefExpression *>(expr.get());

    auto table_catalog_entry = catalog_->GetTable(parser_column_expression->table_name_);
    ChickenException::AssertCondition(table_catalog_entry != nullptr, "[Binder] bound column expression error, unknown table:"+parser_column_expression->table_name_);

    auto schema_page = catalog_->GetSchema(table_catalog_entry->table_id);

    col_id_t id = -1;
    for (auto &column : schema_page->columns_) {
        if (column.col_name ==  parser_column_expression->column_name_) {
            id = column.col_id;
            break;
        }
    }

    ChickenException::AssertCondition(id != -1, "[Binder] bound column error, unknown column:"+parser_column_expression->column_name_);

    return std::make_unique<chickenDB::BoundColumnExpression>(table_catalog_entry->table_id,id);
}

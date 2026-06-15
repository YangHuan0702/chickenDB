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

    // 列名无表限定时（如 SELECT a FROM t），用当前语句的目标表解析。
    const TableCatalogEntry *table_catalog_entry = nullptr;
    if (!parser_column_expression->table_name_.empty()) {
        table_catalog_entry = catalog_->GetTable(parser_column_expression->table_name_);
    } else if (has_current_table_) {
        table_catalog_entry = catalog_->GetTable(current_table_id_);
    }

    ChickenException::AssertCondition(table_catalog_entry != nullptr,
                                      "[Binder] bound column expression error, unknown table: " + parser_column_expression->table_name_);

    auto schema_page = catalog_->GetSchema(table_catalog_entry->table_id);

    col_id_t id = -1;
    // COUNT(*) 等无列名的聚合：默认绑定到第一列（聚合本身只需任一列计数）。
    if (parser_column_expression->column_name_.empty() && parser_column_expression->is_aggregate_) {
        if (!schema_page->columns_.empty()) id = schema_page->columns_[0].col_id;
    } else {
        for (auto &column : schema_page->columns_) {
            if (column.GetColumnName() == parser_column_expression->column_name_) {
                id = column.col_id;
                break;
            }
        }
    }

    ChickenException::AssertCondition(id != -1, "[Binder] bound column error, unknown column:"+parser_column_expression->column_name_);

    auto bound = std::make_unique<chickenDB::BoundColumnExpression>(table_catalog_entry->table_id, id);
    bound->is_aggregate_ = parser_column_expression->is_aggregate_;
    bound->agg_func_ = parser_column_expression->agg_func_;
    return bound;
}

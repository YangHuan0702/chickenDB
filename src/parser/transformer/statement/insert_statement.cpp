//
// Created by huan.yang on 2026-01-27.
//
#include "parser/transformer.h"
#include "parser/statement/insert_statment.h"
#include "parser/tableref/base_tableref.h"

namespace chickenDB {
    std::unique_ptr<SQLStatement> Transformer::TransformerInsert(duckdb_libpgquery::PGNode *node) {
        auto *stmt = reinterpret_cast<duckdb_libpgquery::PGInsertStmt*>(node);

        auto result = std::make_unique<InsertStatement>();

        if (stmt->cols) {
            for (duckdb_libpgquery::ListCell *c = stmt->cols->head; c; c = lnext(c)) {
                auto *target = (duckdb_libpgquery::PGResTarget *)(c->data.ptr_value);
                result->columns.emplace_back(target->name);
            }
        }

        auto select_stmt = reinterpret_cast<duckdb_libpgquery::PGSelectStmt *>(stmt->selectStmt);
        if (!select_stmt->valuesLists) {
            result->select_statement = TransformerSelect(stmt->selectStmt);
        } else {
            TransformValuesList(select_stmt->valuesLists,result->values);
        }

        auto ref = TransformRangeVar(stmt->relation);
        auto &table = *reinterpret_cast<BaseTableRef*> (ref.get());
        result->table = table.table_name;
        result->schema = table.schema_name;
        return nullptr;
    }


    void Transformer::TransformValuesList(duckdb_libpgquery::PGList *list, std::vector<std::vector<std::unique_ptr<ParsedExpression> > > &values) {
        for (auto value_list = list->head; value_list; value_list = value_list->next) {
            auto target = (duckdb_libpgquery::PGList *) (value_list->data.ptr_value);

            std::vector<std::unique_ptr<ParsedExpression>> insert_values;
            if (!TransformerExpressionList(target, insert_values)) {
                throw std::runtime_error("Cloud not parse expression list!");
            }
            if (values.size() > 0) {
                if (values[0].size() != insert_values.size()) {
                    throw std::runtime_error("Values lists must all be the same lenght!");
                }
            }
            values.push_back(std::move(insert_values));
        }
    }

}

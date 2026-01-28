//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"
#include "parser/tableref/sub_query_ref.h"

namespace chickenDB {
    std::unique_ptr<TableRef> Transformer::TransformRangeSubselect(duckdb_libpgquery::PGRangeSubselect *root) {
        auto subQuery = TransformerSelectNode((duckdb_libpgquery::PGSelectStmt*) root->subquery);
        if (!subQuery) {
            return nullptr;
        }

        auto result = std::make_unique<SubqueryRef>(std::move(subQuery));
        result->alias = TransformAlias(root->alias);
        if (root->alias->colnames) {
            for (auto node = root->alias->colnames->head; node; node = node->next) {
                result->column_name_alias.emplace_back(reinterpret_cast<duckdb_libpgquery::PGValue*> (node->data.ptr_value)->val.str);
            }
        }
        return std::move(result);
    }
}

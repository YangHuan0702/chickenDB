//
// Created by huan.yang on 2026-01-27.
//

#include "parser/transformer.h"
#include "parser/tableref/base_tableref.h"

namespace chickenDB {

    std::unique_ptr<TableRef> Transformer::TransformRangeVar(duckdb_libpgquery::PGRangeVar *node) {
        auto result = std::make_unique<BaseTableRef>();

        result->alias = TransformAlias(node->alias);
        if (node->relname) {
            result->table_name = node->relname;
        }
        if (node->schemaname) {
            result->schema_name = node->schemaname;
        }
        return std::move(result);
    }


}
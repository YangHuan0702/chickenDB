//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"
#include "parser/expression/column_expression.h"
#include "parser/expression/star_expression.h"

namespace chickenDB {
    std::unique_ptr<ParsedExpression> Transformer::TransformColumnRef(duckdb_libpgquery::PGColumnRef *root) {
        auto fields = root->fields;
        switch (reinterpret_cast<duckdb_libpgquery::PGNode *>(fields->head->data.ptr_value)->type) {
            case duckdb_libpgquery::T_PGString: {
                if (fields->length < 1 || fields->length > 2) {
                    throw std::runtime_error("Unexpected field length");
                }
                std::string column_name, table_name;
                if (fields->length == 1) {
                    column_name = std::string(reinterpret_cast<duckdb_libpgquery::PGValue*>(fields->head->data.ptr_value)->val.str);
                } else {
                    table_name = std::string(reinterpret_cast<duckdb_libpgquery::PGValue*>(fields->head->data.ptr_value)->val.str);
                    column_name = std::string(reinterpret_cast<duckdb_libpgquery::PGValue*>(fields->head->next->data.ptr_value)->val.str);
                }
                return std::make_unique<ColumnRefExpression>(column_name, table_name);
            }
            case duckdb_libpgquery::T_PGAStar: {
                return std::make_unique<StarExpression>();
            }
            default:
                throw std::runtime_error("Unsupported node type");
        }

    }
}

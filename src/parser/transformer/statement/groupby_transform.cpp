//
// Created by huan.yang on 2026-01-27.
//
#include "parser/transformer.h"

namespace chickenDB {

    bool Transformer::TransformGroupBy(duckdb_libpgquery::PGList *group, std::vector<std::unique_ptr<ParsedExpression> > &result) {
        if (!group) {
            return false;
        }

        for (auto node = group->head; node; node = node->next) {
            duckdb_libpgquery::PGNode *n = reinterpret_cast<duckdb_libpgquery::PGNode*>(node->data.ptr_value);
            result.push_back(TransformExpression(n));
        }
        return true;
    }


}
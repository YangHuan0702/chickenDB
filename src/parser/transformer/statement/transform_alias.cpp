//
// Created by huan.yang on 2026-01-27.
//
#include "parser/transformer.h"
namespace chickenDB {
    std::string Transformer::TransformAlias(duckdb_libpgquery::PGAlias *root) {
        if (!root) {
            return "";
        }
        return root->aliasname;
    }

}
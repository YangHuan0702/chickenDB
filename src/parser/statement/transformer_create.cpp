//
// Created by huan.yang on 2026-04-30.
//
#include "parser/transformer.h"

using namespace chickenDB;


auto Transformer::TransformerCreateTable(const hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    return nullptr;
}

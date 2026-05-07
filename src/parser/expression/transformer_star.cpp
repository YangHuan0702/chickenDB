//
// Created by huan.yang on 2026-05-07.
//
#include "parser/transformer.h"

using namespace chickenDB;

auto Transformer::TransformerStar(hsql::Expr *expr) -> std::unique_ptr<ParserExpression> {
    return nullptr;
}

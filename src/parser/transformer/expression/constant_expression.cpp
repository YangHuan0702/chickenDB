//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"
#include "parser/expression/content_expression.h"

namespace chickenDB {

    std::unique_ptr<ParsedExpression> Transformer::TransformValue(duckdb_libpgquery::PGValue value) {
        switch (value.type) {
            case duckdb_libpgquery::T_PGInteger:
                return std::make_unique<ContentExpression>(SQLType(SQLTypeId::INTEGER),Value::INTEGER((int32_t) value.val.ival));
            case duckdb_libpgquery::T_PGBitString:
            case duckdb_libpgquery::T_PGString:
                return std::make_unique<ContentExpression>(SQLType(SQLTypeId::VARCHAR),Value(std::string(value.val.str)));
            case duckdb_libpgquery::T_PGNull:
                return std::make_unique<ContentExpression>(SQLType(SQLTypeId::SQLNULL),Value());
            default:
                throw std::runtime_error("Unsupported PGValue type");
        }
    }



    std::unique_ptr<ParsedExpression> Transformer::TransformConstant(duckdb_libpgquery::PGAConst *c) {
        return TransformValue(c->val);
    }

}
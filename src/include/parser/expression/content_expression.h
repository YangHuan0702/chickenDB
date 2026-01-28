//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/parsed_expression.h"
#include "common/types/types.h"
#include "common/types/value.h"

namespace chickenDB {
    class ContentExpression : public ParsedExpression {
    public:
        ContentExpression(SQLType sql_type,Value val);

        Value val;
        SQLType sql_type;

        std::string ToString() const override;

        bool Equals(const BaseExpression *other) const override;

        uint64_t Hash() const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

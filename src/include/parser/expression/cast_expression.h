//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "common/types/types.h"
#include "parser/parsed_expression.h"


namespace chickenDB {
    class CastExpression : public ParsedExpression {
    public:
        CastExpression(SQLType target, std::unique_ptr<ParsedExpression> child);

        std::unique_ptr<ParsedExpression> child;
        SQLType cast_type;

        std::string ToString() const override;

        bool Equals(ColumnRefExpression *other) const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

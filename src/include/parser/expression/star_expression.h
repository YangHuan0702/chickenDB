//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/parsed_expression.h"

namespace chickenDB {
    class StarExpression : public ParsedExpression {
    public:
        StarExpression();
        ~StarExpression() override = default;
        std::string GetName() const override { return "StarExpression"; }

        bool Equals(ColumnRefExpression *other) const override { return this == other; }

        bool IsAggregate() const override { return false; }

        bool IsScalar() const override { return false; }

        bool IsWindow() const override { return false; }

        bool HasSubQuery() const override { return false; }

        bool HasParameter() const override { return false; }

        uint64_t Hash() const override { return 0; }

        std::string ToString() const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

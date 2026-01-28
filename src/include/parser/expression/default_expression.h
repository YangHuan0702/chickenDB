//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/parsed_expression.h"

namespace chickenDB {
    class DefaultExpression : public ParsedExpression {
    public:
        DefaultExpression();
        ~DefaultExpression() override = default;
        std::string GetName() const override {
            return "DefaultExpression";
        }

        bool Equals(ColumnRefExpression *other) const override { return false;}

        bool IsAggregate() const override { return false;}

        bool IsWindow() const override { return false;}

        bool HasSubQuery() const override { return false;}

        bool HasParameter() const override { return false;}

        uint64_t Hash() const override { return 0;}

        std::string ToString() const override;

        bool IsScalar() const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

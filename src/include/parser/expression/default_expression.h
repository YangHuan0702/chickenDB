//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/parsed_expression.h"

namespace chickenDB {
    class DefaultExpression : public ParsedExpression {
    public:
        DefaultExpression();

        std::string ToString() const override;

        bool IsScalar() const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

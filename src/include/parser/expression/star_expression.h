//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/parsed_expression.h"

namespace chickenDB {
    class StarExpression : public ParsedExpression {
    public:
        StarExpression();

        std::string ToString() const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

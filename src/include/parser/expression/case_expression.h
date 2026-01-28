//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "common/types/types.h"
#include "parser/parsed_expression.h"

namespace chickenDB {
    class CaseExpression : public ParsedExpression {
    public:
        CaseExpression();
        std::unique_ptr<ParsedExpression> check;
        std::unique_ptr<ParsedExpression> result_if_true;
        std::unique_ptr<ParsedExpression> result_if_false;

        std::unique_ptr<ParsedExpression> child;
        SQLType target_type;

        std::string ToString() const override;

        bool Equals(const BaseExpression *other) const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

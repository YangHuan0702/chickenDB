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


        std::string GetName() const override {
            return "CaseExpression";
        }

        bool IsAggregate() const override {
            return false;
        }

        bool IsScalar() const override {return false;}

        bool IsWindow() const override {return false;}

        bool HasSubQuery() const override {return false;}

        bool HasParameter() const override {return false;}

        uint64_t Hash() const override {return 0;}

        std::string ToString() const override;

        bool Equals(ColumnRefExpression *other) const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

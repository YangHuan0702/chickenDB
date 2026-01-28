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
        ~ContentExpression() override = default;
        Value val;
        SQLType sql_type;

        std::string GetName() const override {
            return "ContentExpression";
        };

        bool IsAggregate() const override {
            return false;
        }

        bool IsScalar() const override {
            return false;
        }

        bool IsWindow() const override {
            return false;
        }

        bool HasSubQuery() const override {
            return false;
        }

        bool HasParameter() const override {
            return false;
        }

        std::string ToString() const override;

        bool Equals(ColumnRefExpression *other) const override;

        uint64_t Hash() const override;

        std::unique_ptr<ParsedExpression> Copy() const override;
    };
}

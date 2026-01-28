//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include "parser/base_expression.h"

namespace chickenDB {

    class ParsedExpression : public BaseExpression {
    public:
        explicit ParsedExpression(ExpressionType type,ExpressionClass expression_class) : BaseExpression(type, expression_class) {}
        ~ParsedExpression() override = default;

        bool IsAggregate() const override;
        bool IsScalar() const override;
        bool IsWindow() const override;
        bool HasSubQuery() const override;
        bool HasParameter() const override;
        uint64_t Hash() const override;

        virtual std::unique_ptr<ParsedExpression> Copy() const = 0;

    protected:
        void CopyProperties(const ParsedExpression &other) {
            type = other.type;
            expressionClass = other.expressionClass;
            alias = other.alias;
        }
    };

}

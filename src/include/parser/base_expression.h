//
// Created by huan.yang on 2026-01-27.
//

#pragma once
#include <memory>

#include "common/enum/expression_type.h"
#include "expression/column_expression.h"

namespace chickenDB {

    class BaseExpression {
    public:
        explicit BaseExpression(ExpressionType type, ExpressionClass expressionClass) : type(type), expressionClass(expressionClass) {}
        virtual ~BaseExpression() = default;

        ExpressionType getType() const { return type;}
        ExpressionClass getExpressionClass() const { return expressionClass;}

        std::string alias;
        ExpressionType type;
        ExpressionClass expressionClass;

        virtual bool IsAggregate() const = 0;
        virtual bool IsWindow() const = 0;
        virtual bool HasSubQuery() const = 0;
        virtual bool IsScalar() const = 0;
        virtual bool HasParameter() const = 0;

        virtual std::string GetName() const {
            return !alias.empty() ? alias : ToString();
        }


        virtual std::string ToString() const = 0;

        virtual uint64_t Hash() const = 0;

        virtual bool Equals(const BaseExpression* other) const;

        static bool Equals(BaseExpression *left, BaseExpression *right) {
            if (left == right) return true;
            if (!left || !right) return false;
            return left->Equals(right);
        }

        bool operator==(const BaseExpression& other) const {
            return this->Equals(&other);
        }
    };

}

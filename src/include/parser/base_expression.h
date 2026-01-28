//
// Created by huan.yang on 2026-01-27.
//

#pragma once
#include <memory>

#include "parsed_expression.h"
#include "common/enum/expression_type.h"

namespace chickenDB {

    class BaseExpression {
    public:
        explicit BaseExpression(ExpressionType type, ExpressionClass expressionClass) : type(type), expressionClass(expressionClass) {}
        virtual ~BaseExpression() {}

        ExpressionType getType() const { return type;}
        ExpressionClass getExpressionClass() const { return expressionClass;}

        std::string alias;
        ExpressionType type;
        ExpressionClass expressionClass;


    public:
        virtual bool IsAggregate() const = 0;
        virtual bool IsWindow() const = 0;
        virtual bool HasSubQuery() const = 0;
        virtual bool IsScalar() const = 0;
        virtual bool HasParameter() const = 0;

        virtual std::string GetName() const {
            return !alias.empty() ? alias : ToString();
        }


        virtual std::string ToString() const = 0;
        void Print();


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
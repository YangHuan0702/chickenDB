//
// Created by huan.yang on 2026-01-28.
//
#include "parser/expression/default_expression.h"

namespace chickenDB {

    std::unique_ptr<ParsedExpression> DefaultExpression::Copy() const {
        auto copy = std::make_unique<DefaultExpression>();
        copy->CopyProperties(*this);
        return std::move(copy);
    }

    std::string DefaultExpression::ToString() const {
        return "Default";
    }

    DefaultExpression::DefaultExpression() : ParsedExpression(ExpressionType::VALUE_DEFAULT, ExpressionClass::DEFAULT) {
    }

    bool DefaultExpression::IsScalar() const {
        return false;
    }





}
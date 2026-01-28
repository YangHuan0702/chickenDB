//
// Created by huan.yang on 2026-01-28.
//
#include "parser/expression/star_expression.h"
namespace chickenDB {

    std::unique_ptr<ParsedExpression> StarExpression::Copy() const {
        auto copy = std::make_unique<StarExpression>();
        copy->CopyProperties(*this);
        return std::move(copy);
    }

    std::string StarExpression::ToString() const {
        return "*";
    }

    StarExpression::StarExpression() : ParsedExpression(ExpressionType::STAR,ExpressionClass::STAR) {
    }




}
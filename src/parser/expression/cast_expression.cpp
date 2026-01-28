//
// Created by huan.yang on 2026-01-28.
//
#include "parser/expression/cast_expression.h"
using namespace chickenDB;

CastExpression::CastExpression(SQLType target, std::unique_ptr<ParsedExpression> child)
    : ParsedExpression(ExpressionType::OPERATOR_CAST, ExpressionClass::CAST), cast_type(target) {
    this->child = std::move(child);
}

std::unique_ptr<ParsedExpression> CastExpression::Copy() const {
    auto copy = std::make_unique<CastExpression>(cast_type, child->Copy());
    copy->CopyProperties(*this);
    return std::move(copy);
}

std::string CastExpression::ToString() const {
    return "CAST[" + std::to_string(cast_type.scale) + "](" + child->ToString() + ")";
}

bool CastExpression::Equals(const BaseExpression *other_) const {
    if (!BaseExpression::Equals(other_)) {
        return false;
    }
    auto other = (CastExpression *) other_;
    if (!child->Equals(other->child.get())) {
        return false;
    }
    return true;
}

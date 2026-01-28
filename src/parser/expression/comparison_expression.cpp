//
// Created by huan.yang on 2026-01-28.
//
#include "parser/expression/comparison_expression.h"

namespace chickenDB {

    ComparisonExpression::ComparisonExpression(ExpressionType type, std::unique_ptr<ParsedExpression> left, std::unique_ptr<ParsedExpression> right)
        : ParsedExpression(type,ExpressionClass::COMPARISON), left(std::move(left)), right(std::move(right)) {
    }

    std::unique_ptr<ParsedExpression> ComparisonExpression::Copy() const {
        auto copy = std::make_unique<ComparisonExpression>(type, left->Copy(), right->Copy());
        copy->CopyProperties(*this);
        return std::move(copy);
    }

    std::string ComparisonExpression::ToString() const {
        return left->ToString() + ExpressionTypeToOperator(type) + right->ToString();
    }


    bool ComparisonExpression::Equals(const BaseExpression *other_) const {
        if (!BaseExpression::Equals(other_)) {
            return false;
        }
        auto other = (ComparisonExpression *)other_;
        if (!left->Equals(other->left.get())) {
            return false;
        }
        if (!right->Equals(other->right.get())) {
            return false;
        }
        return true;
    }

}
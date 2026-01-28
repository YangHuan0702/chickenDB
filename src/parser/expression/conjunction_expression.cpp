//
// Created by huan.yang on 2026-01-28.
//
#include "parser/expression/conjunction_expression.h"

namespace chickenDB {

    ConjunctionExpression::ConjunctionExpression(ExpressionType type, std::unique_ptr<ParsedExpression> left, std::unique_ptr<ParsedExpression> right)
        : ParsedExpression(type,ExpressionClass::CONJUNCTION), left(std::move(left)), right(std::move(right)) {
    }

    std::unique_ptr<ParsedExpression> ConjunctionExpression::Copy() const {
        auto copy = std::make_unique<ConjunctionExpression>(type, left->Copy(), right->Copy());
        copy->CopyProperties(*this);
        return std::move(copy);
    }

    std::string ConjunctionExpression::ToString() const {
        return left->ToString() + " " + ExpressionTypeToOperator(type) + " " + right->ToString();
    }

    bool ConjunctionExpression::Equals(ColumnRefExpression *other_) const {
        auto other = (ConjunctionExpression *)other_;
        if (left->Equals(other->left.get()) && right->Equals(other->right.get())) {
            return true;
        }
        if (right->Equals(other->left.get()) && left->Equals(other->right.get())) {
            return true;
        }
        return false;
    }
}
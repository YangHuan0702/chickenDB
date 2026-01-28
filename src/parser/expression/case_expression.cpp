//
// Created by huan.yang on 2026-01-28.
//
#include "parser/expression/case_expression.h"

using namespace chickenDB;

CaseExpression::CaseExpression() : ParsedExpression(ExpressionType::OPERATOR_CASE_EXPR, ExpressionClass::CASE) {
}

std::unique_ptr<ParsedExpression> CaseExpression::Copy() const {
    auto copy = std::make_unique<CaseExpression>();
    copy->CopyProperties(*this);
    copy->check = check->Copy();
    copy->result_if_true = result_if_true->Copy();
    copy->result_if_false = result_if_false->Copy();
    return std::move(copy);
}

std::string CaseExpression::ToString() const {
    return "CASE WHEN (" + check->ToString() + ") THEN (" + result_if_true->ToString() + ") ELSE (" +
           result_if_false->ToString() + ")";
}

bool CaseExpression::Equals(ColumnRefExpression *other_) const {
    auto other = (CaseExpression *)other_;
    if (!check->Equals(other->check.get())) {
        return false;
    }
    if (!result_if_true->Equals(other->result_if_true.get())) {
        return false;
    }
    if (!result_if_false->Equals(other->result_if_false.get())) {
        return false;
    }
    return true;
}



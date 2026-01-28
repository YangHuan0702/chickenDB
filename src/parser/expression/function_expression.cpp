//
// Created by huan.yang on 2026-01-28.
//
#include "parser/expression/function_expression.h"
#include "common/constants.h"
#include "parser/expression/column_expression.h"

using namespace chickenDB;

FunctionExpression::FunctionExpression(std::string schema, std::string function_name,
                                       std::vector<std::unique_ptr<ParsedExpression>> &children)
    : ParsedExpression(ExpressionType::FUNCTION, ExpressionClass::FUNCTION), schema(schema),
      function_name(function_name) {
    for (auto &child : children) {
        this->children.push_back(std::move(child));
    }
}

FunctionExpression::FunctionExpression(std::string function_name, std::vector<std::unique_ptr<ParsedExpression> > &children) : FunctionExpression(DEFAULT_SCHEMA, function_name, children) {
}

std::unique_ptr<ParsedExpression> FunctionExpression::Copy() const {
    std::vector<std::unique_ptr<ParsedExpression>> copy_children;
    for (auto &child : children) {
        copy_children.push_back(child->Copy());
    }
    auto copy = std::make_unique<FunctionExpression>(function_name, copy_children);
    copy->schema = schema;
    copy->CopyProperties(*this);
    return std::move(copy);
}

bool FunctionExpression::Equals(ColumnRefExpression *other_) const {
    auto other = (FunctionExpression *)other_;
    if (schema != other->schema && function_name != other->function_name) {
        return false;
    }
    if (other->children.size() != children.size()) {
        return false;
    }
    for (index_t i = 0; i < children.size(); i++) {
        if (!children[i]->Equals(other->children[i].get())) {
            return false;
        }
    }
    return true;
}


std::string FunctionExpression::ToString() const {
    std::string result = function_name + "(";
    for (index_t i = 0; i < children.size(); i++) {
        result += children[i]->ToString() + (i + 1 == children.size() ? ")" : ",");
    }
    return result;
}


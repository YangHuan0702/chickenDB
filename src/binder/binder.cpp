//
// Created by huan.yang on 2026-05-08.
//
#include "binder/binder.h"

using namespace chickenDB;

auto Binder::BinderStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement> {
    switch (statement->type_) {
        case StatementType::CREATE: return BinderCreateStatement(std::move(statement));
        case StatementType::CREATE_INDEX: return BinderCreateIndexStatement(std::move(statement));
        case StatementType::SELECT: return BinderSelectStatement(std::move(statement));
        case StatementType::INSERT: return BinderInsertStatement(std::move(statement));
        case StatementType::DELETE: return BinderDeleteStatement(std::move(statement));
        case StatementType::UPDATE: return BinderUpdateStatement(std::move(statement));
        default: throw std::runtime_error("Binder::BinderStatement: invalid statement");
    }
}


auto Binder::BinderStatement(std::vector<std::unique_ptr<SQLStatement> > statements) -> std::vector<std::unique_ptr<BoundStatement>> {
    std::vector<std::unique_ptr<BoundStatement>> r;
    if (statements.empty()) {
        return r;
    }

    for (auto &sql_statement : statements) {
        r.push_back(BinderStatement(std::move(sql_statement)));
    }
    return r;
}


auto Binder::BoundExpression(std::unique_ptr<ParserExpression> expression) -> std::unique_ptr<chickenDB::BoundExpression> {
    switch (expression->type_) {
        case ParserExpressionType::BINARY_OP: return BoundBinaryExpression(std::move(expression));
        case ParserExpressionType::COLUMN: return BoundColumnExpression(std::move(expression));
        case ParserExpressionType::CONSTANT: return BoundConstantExpression(std::move(expression));
        case ParserExpressionType::UNARY_OP: return BoundUnaryExpression(std::move(expression));
        default: throw std::runtime_error("Binder::BindExpression: invalid expression type");
    }
}

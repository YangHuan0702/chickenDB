//
// Created by huan.yang on 2026-05-07.
//
#pragma once
#include "catalog/catalog.h"
#include "expression/bound_expression.h"
#include "parser/expression/expression.h"
#include "statement/bound_statement.h"

namespace chickenDB {
    class Binder {
    public:
        explicit Binder(std::shared_ptr<Catalog> catalog) : catalog_(catalog) {
        }

        virtual ~Binder() = default;

        auto BinderStatement(std::vector<std::unique_ptr<SQLStatement> > statements) -> std::vector<std::unique_ptr<BoundStatement>>;
        auto BinderStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement>;
        auto BinderSelectStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement>;
        auto BinderInsertStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement>;
        auto BinderDeleteStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement>;
        auto BinderUpdateStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement>;
        auto BinderCreateStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement>;
        auto BinderCreateIndexStatement(std::unique_ptr<SQLStatement> statement) -> std::unique_ptr<BoundStatement>;


        auto BoundExpression(std::unique_ptr<ParserExpression> expression) -> std::unique_ptr<BoundExpression>;
        auto BoundBinaryExpression(std::unique_ptr<ParserExpression>) -> std::unique_ptr<class BoundExpression>;
        auto BoundColumnExpression(std::unique_ptr<ParserExpression> expr) -> std::unique_ptr<class BoundExpression>;
        auto BoundConstantExpression(std::unique_ptr<ParserExpression>) -> std::unique_ptr<class BoundExpression>;
        auto BoundUnaryExpression(std::unique_ptr<ParserExpression>) -> std::unique_ptr<class BoundExpression>;


        std::vector<std::unique_ptr<BoundStatement> > bound_statements_;
        std::shared_ptr<Catalog> catalog_;

        // 当前语句的目标表（解析无表限定的列名时用，如 SELECT a FROM t 中的 a）。
        table_id_t current_table_id_{0};
        bool has_current_table_{false};
    };
}

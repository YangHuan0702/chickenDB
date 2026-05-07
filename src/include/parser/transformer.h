//
// Created by huan.yang on 2026-04-30.
//

#pragma once
#include "SQLParserResult.h"
#include "expression/expression.h"
#include "sql/CreateStatement.h"
#include "statment/sql_statement.h"


namespace chickenDB {
    class Transformer {
    public:
        auto TransformerAST(hsql::SQLParserResult &result,
                            std::vector<std::unique_ptr<SQLStatement> > &statment) -> void;

        auto TransformerStatement(const hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;

        auto TransformerSelectStatement(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;
        auto TransformerInsertStatement( hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;
        auto TransformerUpdateStatement( hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;
        auto TransformerDeleteStatement( hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;
        auto TransformerCreateTable(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;


        auto TransformerExpression(hsql::Expr *) -> std::unique_ptr<ParserExpression>;
        auto TransformerOperatorExpression(hsql::Expr *) -> std::unique_ptr<ParserExpression>;
        auto TransformerBinaryOperator(hsql::Expr *) -> std::unique_ptr<ParserExpression>;
        auto TransformerUnaryOperator(hsql::Expr *) -> std::unique_ptr<ParserExpression>;
        auto TransformerStar(hsql::Expr *) -> std::unique_ptr<ParserExpression>;
        auto TransformerColumnRef(hsql::Expr *) -> std::unique_ptr<ParserExpression>;
        auto TransformerConstant(hsql::Expr *) -> std::unique_ptr<ParserExpression>;


    };
}

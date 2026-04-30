//
// Created by huan.yang on 2026-04-30.
//

#pragma once
#include "SQLParserResult.h"
#include "statment/sql_statement.h"


namespace chickenDB {
    class Transformer {
    public:
        auto TransformerAST(hsql::SQLParserResult &result,
                            std::vector<std::unique_ptr<SQLStatement> > &statment) -> void;

        auto TransformerStatement(const hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;

        auto TransformerSelectStatement(const hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;
        auto TransformerInsertStatement(const hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;
        auto TransformerUpdateStatement(const hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;
        auto TransformerDeleteStatement(const hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;
        auto TransformerCreateTable(const hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement>;

    };
}

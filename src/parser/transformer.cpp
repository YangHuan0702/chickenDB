//
// Created by huan.yang on 2026-01-27.
//

#include "parser/transformer.h"
#include "glog/logging.h"
#include "postgres_parser.hpp"
#include "common/chicken_execption.h"
#include "nodes/parsenodes.hpp"

using namespace chickenDB;

auto Transformer::TransformerAST(hsql::SQLParserResult &result,
                                 std::vector<std::unique_ptr<SQLStatement> > &statments) -> void {
    auto size = result.size();

    for (size_t i = 0; i < size; i++) {
        const hsql::SQLStatement *state = result.getStatement(i);
        std::unique_ptr<SQLStatement> statement = TransformerStatement(state);
        statments.push_back(std::move(statement));
    }
}

auto Transformer::TransformerStatement(const hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    switch (statement->type()) {
        case hsql::StatementType::kStmtSelect: return TransformerSelectStatement(statement);
        case hsql::StatementType::kStmtInsert: return TransformerInsertStatement(statement);
        case hsql::StatementType::kStmtUpdate: return TransformerUpdateStatement(statement);
        case hsql::StatementType::kStmtDelete: return TransformerDeleteStatement(statement);
        case hsql::StatementType::kStmtCreate: return TransformerCreateTable(statement);
        default: throw ChickenException("TransformerStatement: invalid statement type");
    }
}

//
// Created by huan.yang on 2026-06-11.
//
#include "common/chicken_execption.h"
#include "parser/transformer.h"
#include "parser/statment/transaction_statement.h"
#include "sql/TransactionStatement.h"

using namespace chickenDB;

auto Transformer::TransformerTransaction(hsql::SQLStatement *statement) -> std::unique_ptr<SQLStatement> {
    auto *txn_stmt = dynamic_cast<hsql::TransactionStatement *>(statement);
    switch (txn_stmt->command) {
        case hsql::kBeginTransaction:
            return std::make_unique<TransactionStatement>(TransactionCommand::BEGIN);
        case hsql::kCommitTransaction:
            return std::make_unique<TransactionStatement>(TransactionCommand::COMMIT);
        case hsql::kRollbackTransaction:
            return std::make_unique<TransactionStatement>(TransactionCommand::ROLLBACK);
        default:
            throw ChickenException("[Transformer] unknown transaction command");
    }
}

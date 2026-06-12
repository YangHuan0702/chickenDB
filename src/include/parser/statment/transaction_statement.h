//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include "sql_statement.h"

namespace chickenDB {
    enum class TransactionCommand {
        BEGIN,
        COMMIT,
        ROLLBACK,
    };

    // BEGIN / COMMIT / ROLLBACK。事务控制语句作用于会话的当前事务，不进入算子流水线。
    class TransactionStatement : public SQLStatement {
    public:
        explicit TransactionStatement(TransactionCommand command)
            : SQLStatement(StatementType::TRANSACTION), command_(command) {}
        ~TransactionStatement() override = default;

        TransactionCommand command_;
    };
}

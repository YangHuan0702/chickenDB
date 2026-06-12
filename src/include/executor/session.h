//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "buffer/buffer_manager.h"
#include "catalog/catalog.h"
#include "common/value.h"
#include "transaction/transaction.h"
#include "transaction/transaction_manager.h"
#include "transaction/version_store.h"
#include "transaction/log_manager.h"

namespace chickenDB {
    // 会话：把事务子系统（管理器/版本存储/WAL）与 catalog/buffer 绑在一起，按语句维持
    // 当前事务，提供 SQL 执行入口。它是事务跨语句存活的归属（执行器每条语句新建上下文，
    // 但 txn 由会话持有并注入）。
    //
    // 语义：
    //  - 显式事务：BEGIN 开启，后续语句共用该事务，直到 COMMIT/ROLLBACK。
    //  - 自动提交：未显式 BEGIN 时，每条 DML 在独立事务中执行并立即提交。
    class Session {
    public:
        Session(std::shared_ptr<BufferManager> buffer, std::shared_ptr<Catalog> catalog,
                std::shared_ptr<TransactionManager> txn_manager,
                std::shared_ptr<VersionStore> version_store,
                std::shared_ptr<LogManager> log_manager);

        // 执行一条 SQL。SELECT 等查询结果存入 last_result_。
        auto Execute(const std::string &sql) -> void;

        auto LastResult() const -> const std::vector<std::vector<Value>> & { return last_result_; }

    private:
        auto BeginTxn() -> void;
        auto CommitTxn() -> void;
        auto AbortTxn() -> void;
        // 提交时把本事务创建/删除的行版本时间戳回填，并写 COMMIT 日志。
        auto FinalizeCommit(const std::shared_ptr<Transaction> &txn) -> void;

        std::shared_ptr<BufferManager> buffer_;
        std::shared_ptr<Catalog> catalog_;
        std::shared_ptr<TransactionManager> txn_manager_;
        std::shared_ptr<VersionStore> version_store_;
        std::shared_ptr<LogManager> log_manager_;

        std::shared_ptr<Transaction> current_txn_{nullptr};
        bool in_explicit_txn_{false};
        std::vector<std::vector<Value>> last_result_;
    };
}

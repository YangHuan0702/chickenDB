//
// Created by huan.yang on 2026-05-21.
//
#pragma once
#include <memory>

#include "buffer/buffer_manager.h"
#include "catalog/catalog.h"
#include "planner/physical/physical_operator.h"
#include "transaction/transaction.h"
#include "transaction/transaction_manager.h"
#include "transaction/version_store.h"
#include "transaction/log_manager.h"

namespace chickenDB {
    class ExecutorContext {
    public:
        explicit ExecutorContext(std::shared_ptr<BufferManager> buffer_manager,
                                 std::shared_ptr<Catalog> catalog) : buffer_manager_(buffer_manager),
                                                                     catalog_(catalog) {
        }
        ~ExecutorContext() = default;

        std::shared_ptr<BufferManager> buffer_manager_;
        std::shared_ptr<Catalog> catalog_;

        // 事务子系统（可选）：未设置时执行器走非事务路径（兼容旧行为）。
        // txn_ 为当前语句所属事务；三个管理器通常由会话/数据库实例共享。
        std::shared_ptr<TransactionManager> txn_manager_{nullptr};
        std::shared_ptr<VersionStore> version_store_{nullptr};
        std::shared_ptr<LogManager> log_manager_{nullptr};
        std::shared_ptr<Transaction> txn_{nullptr};

        auto HasTxn() const -> bool {
            return txn_ != nullptr && version_store_ != nullptr;
        }
    };
}

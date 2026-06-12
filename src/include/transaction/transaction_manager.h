//
// Created by huan.yang on 2026-05-22.
//
#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "transaction/transaction.h"

namespace chickenDB {
    // 事务管理器：分配事务 ID 与提交时间戳，维护活跃事务表。
    //
    // 时间戳模型（快照隔离）：txn_id 与 commit_ts 各自单调递增。Begin 时事务拍下
    // read_ts = 当前 last_commit_ts_，只能看见 commit_ts <= read_ts 的已提交版本。
    // Commit 时分配新的 commit_ts 并推进 last_commit_ts_。
    class TransactionManager {
    public:
        explicit TransactionManager() = default;
        ~TransactionManager() = default;

        // 兼容旧接口：仅分配一个事务 ID。
        auto GetNextTxId() -> txn_id_t { return next_txn_id_.fetch_add(1); }

        // 开启事务：分配 id + 快照 read_ts，登记到活跃表。
        auto Begin() -> std::shared_ptr<Transaction>;

        // 提交：分配 commit_ts，标记 COMMITTED，推进 last_commit_ts_，移出活跃表。
        auto Commit(const std::shared_ptr<Transaction> &txn) -> void;

        // 回滚：标记 ABORTED，移出活跃表（版本回滚由上层 version store 处理）。
        auto Abort(const std::shared_ptr<Transaction> &txn) -> void;

        auto LastCommitTs() const -> timestamp_t { return last_commit_ts_.load(); }

        // 恢复后用：把计数器对齐到持久化的最大值之上。
        auto Reset(txn_id_t next_txn, timestamp_t last_commit) -> void;

    private:
        std::atomic<txn_id_t> next_txn_id_{1};
        std::atomic<timestamp_t> next_commit_ts_{1};
        std::atomic<timestamp_t> last_commit_ts_{0};

        std::mutex mutex_;
        std::unordered_map<txn_id_t, std::shared_ptr<Transaction>> active_txns_;
    };
}

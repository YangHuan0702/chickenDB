//
// Created by huan.yang on 2026-06-11.
//
#pragma once

#include "transaction/log_manager.h"
#include "transaction/version_store.h"
#include "transaction/transaction_manager.h"

namespace chickenDB {
    // 崩溃恢复：回放 WAL 重建内存版 version store（B 方案版本信息在页外，故恢复=重放）。
    //
    // 单遍扫描日志：先确定每个事务是否提交（及其 commit_ts），再按记录类型重放——
    //  - 已提交事务的 INSERT：在 version store 建版本，begin_ts = 该事务 commit_ts。
    //  - 已提交事务的 DELETE：标记 end_ts = 该事务 commit_ts。
    //  - 未提交（无 COMMIT）事务的写：丢弃（相当于 undo，不重建其版本）。
    // 同时把 TransactionManager 的计数器对齐到日志中见过的最大 txn_id / commit_ts。
    class RecoveryManager {
    public:
        RecoveryManager(LogManager *log, VersionStore *version_store, TransactionManager *txn_manager)
            : log_(log), version_store_(version_store), txn_manager_(txn_manager) {}

        // 执行恢复。返回重放的已提交事务数。
        auto Recover() -> size_t;

    private:
        LogManager *log_;
        VersionStore *version_store_;
        TransactionManager *txn_manager_;
    };
}

//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <mutex>
#include <unordered_map>

#include "common/rid.h"
#include "transaction/transaction.h"

namespace chickenDB {
    // 页外版本元数据（B 方案）：以 RID 为 key，记录该行的版本可见性信息，
    // 不侵入列式压缩页格式。一行一条 VersionMeta。
    //
    // 字段语义（快照隔离）：
    //  - begin_txn / begin_ts：创建该行的事务及其提交时间戳（未提交时 begin_ts=INVALID）。
    //  - end_txn / end_ts：删除该行的事务及其提交时间戳；未删除时 end_txn=INVALID。
    // 可见性：对快照 read_ts，行可见 当且仅当 begin 已提交且 begin_ts<=read_ts，
    //         且（未删除 或 删除尚未提交 或 end_ts>read_ts）。
    struct VersionMeta {
        txn_id_t begin_txn{INVALID_TXN_ID};
        timestamp_t begin_ts{INVALID_TS};
        txn_id_t end_txn{INVALID_TXN_ID};
        timestamp_t end_ts{INVALID_TS};
    };

    class VersionStore {
    public:
        explicit VersionStore() = default;

        // 插入新行：登记创建事务（begin_ts 暂为 INVALID，提交时回填）。
        auto OnInsert(const RID &rid, txn_id_t txn) -> void;

        // 删除行：登记删除事务（end_ts 暂为 INVALID，提交时回填）。
        auto OnDelete(const RID &rid, txn_id_t txn) -> void;

        // 提交收尾：把该事务创建/删除的行的对应时间戳回填为 commit_ts。
        auto CommitInsert(const RID &rid, timestamp_t commit_ts) -> void;
        auto CommitDelete(const RID &rid, timestamp_t commit_ts) -> void;

        // 回滚收尾：撤销该事务对该行的创建/删除标记。
        auto AbortInsert(const RID &rid) -> void;
        auto AbortDelete(const RID &rid) -> void;

        // 可见性判断：行（rid）对事务 txn 的快照是否可见。
        auto IsVisible(const RID &rid, const Transaction &txn) const -> bool;

        // 恢复用：直接设置某行的版本元数据。
        auto SetMeta(const RID &rid, const VersionMeta &meta) -> void;
        auto GetMeta(const RID &rid, VersionMeta &out) const -> bool;

    private:
        mutable std::mutex mutex_;
        std::unordered_map<RID, VersionMeta, RidHash> store_;
    };
}

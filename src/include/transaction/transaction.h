//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <cstdint>
#include <vector>

#include "common/rid.h"

namespace chickenDB {
    using txn_id_t = int64_t;
    using timestamp_t = int64_t;

    constexpr txn_id_t INVALID_TXN_ID = -1;
    constexpr timestamp_t INVALID_TS = -1;

    enum class TransactionState {
        RUNNING,
        COMMITTED,
        ABORTED,
    };

    // 一次事务的运行期状态。read_ts 是 Begin 时拍下的快照（= 当时最后提交的 commit_ts），
    // 决定本事务能看见哪些版本（快照隔离）。write_set 记录本事务写过的 RID，用于
    // commit/abort 时收尾（标记版本 commit_ts 或回滚）。
    class Transaction {
    public:
        explicit Transaction(txn_id_t id, timestamp_t read_ts) : txn_id_(id), read_ts_(read_ts) {}

        auto GetTxnId() const -> txn_id_t { return txn_id_; }
        auto GetReadTs() const -> timestamp_t { return read_ts_; }
        auto GetState() const -> TransactionState { return state_; }
        auto SetState(TransactionState s) -> void { state_ = s; }
        auto GetCommitTs() const -> timestamp_t { return commit_ts_; }
        auto SetCommitTs(timestamp_t ts) -> void { commit_ts_ = ts; }

        // 记录本事务新建（insert）和删除（delete）的行，供提交/回滚收尾。
        auto AppendInsert(const RID &rid) -> void { insert_set_.push_back(rid); }
        auto AppendDelete(const RID &rid) -> void { delete_set_.push_back(rid); }
        auto InsertSet() const -> const std::vector<RID> & { return insert_set_; }
        auto DeleteSet() const -> const std::vector<RID> & { return delete_set_; }

    private:
        txn_id_t txn_id_;
        timestamp_t read_ts_;
        timestamp_t commit_ts_{INVALID_TS};
        TransactionState state_{TransactionState::RUNNING};
        std::vector<RID> insert_set_;
        std::vector<RID> delete_set_;
    };
}

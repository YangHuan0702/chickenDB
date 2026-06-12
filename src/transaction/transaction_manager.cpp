//
// Created by huan.yang on 2026-06-11.
//
#include "transaction/transaction_manager.h"

using namespace chickenDB;

auto TransactionManager::Begin() -> std::shared_ptr<Transaction> {
    const txn_id_t id = next_txn_id_.fetch_add(1);
    const timestamp_t read_ts = last_commit_ts_.load();
    auto txn = std::make_shared<Transaction>(id, read_ts);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_txns_[id] = txn;
    }
    return txn;
}

auto TransactionManager::Commit(const std::shared_ptr<Transaction> &txn) -> void {
    const timestamp_t commit_ts = next_commit_ts_.fetch_add(1);
    txn->SetCommitTs(commit_ts);
    txn->SetState(TransactionState::COMMITTED);

    // 推进 last_commit_ts_ 到 >= commit_ts（单调）。
    timestamp_t prev = last_commit_ts_.load();
    while (prev < commit_ts && !last_commit_ts_.compare_exchange_weak(prev, commit_ts)) {
        // retry
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_txns_.erase(txn->GetTxnId());
    }
}

auto TransactionManager::Abort(const std::shared_ptr<Transaction> &txn) -> void {
    txn->SetState(TransactionState::ABORTED);
    std::lock_guard<std::mutex> lock(mutex_);
    active_txns_.erase(txn->GetTxnId());
}

auto TransactionManager::Reset(txn_id_t next_txn, timestamp_t last_commit) -> void {
    next_txn_id_.store(next_txn);
    last_commit_ts_.store(last_commit);
    next_commit_ts_.store(last_commit + 1);
}

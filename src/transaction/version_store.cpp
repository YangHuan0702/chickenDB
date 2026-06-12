//
// Created by huan.yang on 2026-06-11.
//
#include "transaction/version_store.h"

using namespace chickenDB;

auto VersionStore::OnInsert(const RID &rid, txn_id_t txn) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    VersionMeta meta;
    meta.begin_txn = txn;
    meta.begin_ts = INVALID_TS; // 提交时回填
    store_[rid] = meta;
}

auto VersionStore::OnDelete(const RID &rid, txn_id_t txn) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(rid);
    if (it == store_.end()) return;
    it->second.end_txn = txn;
    it->second.end_ts = INVALID_TS; // 提交时回填
}

auto VersionStore::CommitInsert(const RID &rid, timestamp_t commit_ts) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(rid);
    if (it != store_.end()) {
        it->second.begin_ts = commit_ts;
    }
}

auto VersionStore::CommitDelete(const RID &rid, timestamp_t commit_ts) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(rid);
    if (it != store_.end()) {
        it->second.end_ts = commit_ts;
    }
}

auto VersionStore::AbortInsert(const RID &rid) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    store_.erase(rid); // 创建被回滚：该行从此不可见
}

auto VersionStore::AbortDelete(const RID &rid) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(rid);
    if (it != store_.end()) {
        it->second.end_txn = INVALID_TXN_ID; // 撤销删除标记
        it->second.end_ts = INVALID_TS;
    }
}

// 快照隔离可见性：
//  1) begin 必须已提交（begin_ts != INVALID）且 begin_ts <= read_ts；
//     例外：本事务自己刚插入但还没提交的行，对自己可见。
//  2) 未被删除，或删除尚未提交，或删除提交时间 > read_ts；
//     例外：本事务自己删的行，对自己不可见。
auto VersionStore::IsVisible(const RID &rid, const Transaction &txn) const -> bool {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(rid);
    if (it == store_.end()) {
        // 无版本元数据：视为旧数据（在事务系统启用前写入），默认可见。
        return true;
    }
    const VersionMeta &m = it->second;
    const timestamp_t read_ts = txn.GetReadTs();
    const txn_id_t self = txn.GetTxnId();

    // 创建可见性。
    bool begin_visible;
    if (m.begin_txn == self) {
        begin_visible = true; // 自己创建的，对自己可见
    } else if (m.begin_ts == INVALID_TS) {
        begin_visible = false; // 别的事务未提交的创建，不可见
    } else {
        begin_visible = m.begin_ts <= read_ts;
    }
    if (!begin_visible) return false;

    // 删除可见性：若已被删除且对本快照生效，则不可见。
    if (m.end_txn != INVALID_TXN_ID) {
        if (m.end_txn == self) {
            return false; // 自己删的，对自己不可见
        }
        if (m.end_ts != INVALID_TS && m.end_ts <= read_ts) {
            return false; // 删除已提交且在快照之前
        }
    }
    return true;
}

auto VersionStore::SetMeta(const RID &rid, const VersionMeta &meta) -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    store_[rid] = meta;
}

auto VersionStore::GetMeta(const RID &rid, VersionMeta &out) const -> bool {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(rid);
    if (it == store_.end()) return false;
    out = it->second;
    return true;
}

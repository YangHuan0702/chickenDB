//
// Created by huan.yang on 2026-06-11.
//
#include "transaction/recovery_manager.h"

#include <unordered_map>
#include <vector>

using namespace chickenDB;

auto RecoveryManager::Recover() -> size_t {
    std::vector<LogRecord> records = log_->ReadAll();
    if (records.empty()) {
        return 0;
    }

    // 第一遍：收集每个事务的提交时间戳（仅 COMMIT 过的事务有效）。
    std::unordered_map<txn_id_t, timestamp_t> committed; // txn_id -> commit_ts
    txn_id_t max_txn = 0;
    timestamp_t max_commit = 0;
    for (const auto &r : records) {
        if (r.txn_id > max_txn) max_txn = r.txn_id;
        if (r.type == LogRecordType::COMMIT) {
            committed[r.txn_id] = r.commit_ts;
            if (r.commit_ts > max_commit) max_commit = r.commit_ts;
        }
    }

    // 第二遍：重放已提交事务的 INSERT/DELETE 到 version store。
    for (const auto &r : records) {
        if (r.type != LogRecordType::INSERT && r.type != LogRecordType::DELETE) {
            continue;
        }
        auto cit = committed.find(r.txn_id);
        if (cit == committed.end()) {
            continue; // 未提交事务的写：丢弃（undo）
        }
        const timestamp_t commit_ts = cit->second;
        const RID rid(r.rid_page, r.rid_row);

        if (r.type == LogRecordType::INSERT) {
            VersionMeta meta;
            // 若该行已被恢复出（之前的 INSERT），保留其 begin，补/盖删除信息时再处理。
            version_store_->GetMeta(rid, meta);
            meta.begin_txn = r.txn_id;
            meta.begin_ts = commit_ts;
            version_store_->SetMeta(rid, meta);
        } else { // DELETE
            VersionMeta meta;
            version_store_->GetMeta(rid, meta);
            meta.end_txn = r.txn_id;
            meta.end_ts = commit_ts;
            version_store_->SetMeta(rid, meta);
        }
    }

    // 对齐事务管理器计数器，避免恢复后复用旧 id/ts。
    txn_manager_->Reset(max_txn + 1, max_commit);
    return committed.size();
}

//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <cstdint>

#include "common/rid.h"
#include "transaction/transaction.h"

namespace chickenDB {
    using lsn_t = int64_t;
    constexpr lsn_t INVALID_LSN = -1;

    enum class LogRecordType : uint8_t {
        INVALID = 0,
        BEGIN = 1,
        INSERT = 2,
        DELETE = 3,
        COMMIT = 4,
        ABORT = 5,
    };

    // 一条 WAL 日志记录（定长，便于顺序读写）。payload 对 INSERT/DELETE 记录受影响
    // 的 RID 与所属表；COMMIT 记录提交时间戳。先写日志后改数据（write-ahead）。
    struct LogRecord {
        lsn_t lsn{INVALID_LSN};
        LogRecordType type{LogRecordType::INVALID};
        txn_id_t txn_id{INVALID_TXN_ID};
        table_id_t table_id{0};
        page_id_t rid_page{-1};
        uint32_t rid_row{0};
        timestamp_t commit_ts{INVALID_TS};
    };
}

//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <mutex>
#include <string>
#include <vector>

#include "transaction/log_record.h"

namespace chickenDB {
    // WAL 日志管理器：把定长 LogRecord 顺序追加到日志文件，并支持全量回读（恢复用）。
    // write-ahead：数据页落盘前，对应日志必须先 Append+Flush。
    //
    // 日志文件路径默认在数据目录下（GetDataPath()/wal.log），可在构造时覆盖。
    class LogManager {
    public:
        explicit LogManager(std::string log_path = "");
        ~LogManager();

        // 追加一条记录，分配并返回 LSN。线程安全。
        auto Append(LogRecord record) -> lsn_t;

        // 把缓冲的日志刷到磁盘（fsync 语义）。
        auto Flush() -> void;

        // 全量回读日志（恢复时用），按 LSN 顺序返回。
        auto ReadAll() -> std::vector<LogRecord>;

        auto CurrentLsn() const -> lsn_t { return next_lsn_ - 1; }

    private:
        auto OpenFile() -> void;

        std::string log_path_;
        int fd_{-1};
        lsn_t next_lsn_{0};
        std::mutex mutex_;
    };
}

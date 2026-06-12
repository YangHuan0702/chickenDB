//
// Created by huan.yang on 2026-06-11.
//
#include "transaction/log_manager.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>

#include "common/constants.h"

using namespace chickenDB;

LogManager::LogManager(std::string log_path) : log_path_(std::move(log_path)) {
    if (log_path_.empty()) {
        log_path_ = GetDataPath() + "/wal.log";
    }
    OpenFile();
    // 用已有记录数推算下一个 LSN。
    struct stat st{};
    if (fstat(fd_, &st) == 0 && st.st_size > 0) {
        next_lsn_ = static_cast<lsn_t>(st.st_size / static_cast<long>(sizeof(LogRecord)));
    }
}

LogManager::~LogManager() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

auto LogManager::OpenFile() -> void {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(log_path_).parent_path(), ec);
    fd_ = ::open(log_path_.c_str(), O_RDWR | O_CREAT, 0644);
}

auto LogManager::Append(LogRecord record) -> lsn_t {
    std::lock_guard<std::mutex> lock(mutex_);
    const lsn_t lsn = next_lsn_++;
    record.lsn = lsn;
    const off_t offset = static_cast<off_t>(lsn) * static_cast<off_t>(sizeof(LogRecord));
    ::pwrite(fd_, &record, sizeof(LogRecord), offset);
    return lsn;
}

auto LogManager::Flush() -> void {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ >= 0) {
        ::fsync(fd_);
    }
}

auto LogManager::ReadAll() -> std::vector<LogRecord> {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LogRecord> out;
    struct stat st{};
    if (fstat(fd_, &st) != 0 || st.st_size == 0) {
        return out;
    }
    const size_t count = static_cast<size_t>(st.st_size) / sizeof(LogRecord);
    out.resize(count);
    for (size_t i = 0; i < count; i++) {
        const off_t offset = static_cast<off_t>(i) * static_cast<off_t>(sizeof(LogRecord));
        ::pread(fd_, &out[i], sizeof(LogRecord), offset);
    }
    return out;
}

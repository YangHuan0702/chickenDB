//
// Created by huan.yang on 2026-05-08.
//
#include "disk/table_manager.h"

#include <fstream>
#include <sstream>

#include "common/chicken_execption.h"
#include "common/constants.h"
#include "common/utils/string_utils.h"

using namespace chickenDB;

LRUTableManager::LRUTableManager(uint32_t capacity) : file_lru_manager_(capacity) {
    files_.reserve(capacity);
    for (size_t i = 0; i < capacity; i++) {
        free_.push_back(i);
    }
}

LRUTableManager::~LRUTableManager() {
    files_.clear();
}

auto GetFilePath(table_id_t table_id) -> std::string {
    std::stringstream ss;
    ss << DATA_PATH << "/" << StringUtil::FormatFileName(table_id);
    return ss.str();
}


auto LRUTableManager::InitFile(table_id_t table_id, size_t frame_id) -> void {
    files_[frame_id] = std::make_shared<std::fstream>();
    std::string file_path = GetFilePath(table_id);

    files_[frame_id]->open(file_path, std::ios::in | std::ios::out | std::ios::binary);

    ChickenException::AssertCondition(files_[frame_id]->is_open(), "Could not open file " + file_path);
}


auto LRUTableManager::Acquire(table_id_t table_id) -> std::shared_ptr<std::fstream> {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t frame_id = 0;
    if (tables_.find(table_id) == tables_.end()) {
        // open
        if (!free_.empty()) {
            frame_id = free_.back();
            free_.pop_back();
        } else {
            frame_id = file_lru_manager_.Evict();
        }

        if (files_[frame_id] != nullptr) {
            files_[frame_id]->close();
        }

        InitFile(table_id, frame_id);
    } else {
        frame_id = tables_[table_id];
    }
    file_lru_manager_.Pin(frame_id);
    return files_[frame_id];
}


auto LRUTableManager::Release(table_id_t table_id) -> void {
    std::lock_guard<std::mutex> lock(mutex_);

    if (tables_.find(table_id) == tables_.end()) {
        return;
    }

    size_t frame_id = tables_[table_id];
    file_lru_manager_.Unpin(frame_id);

    if (file_lru_manager_.GetPinCount(frame_id) <= 0) {
        free_.push_back(frame_id);
    }
}

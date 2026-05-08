//
// Created by huan.yang on 2026-05-08.
//
#include "disk/table_manager.h"

#include <filesystem>
#include <fstream>
#include <system_error>
#include <sstream>

#include "common/chicken_execption.h"
#include "common/constants.h"
#include "common/utils/string_utils.h"

using namespace chickenDB;

LRUTableManager::LRUTableManager(uint32_t capacity) : file_lru_manager_(capacity) {
    ChickenException::AssertCondition(capacity > 0, "file lru capacity must be greater than 0");

    files_.resize(capacity);
    for (size_t i = 0; i < capacity; i++) {
        free_.push_back(i);
    }
}

LRUTableManager::~LRUTableManager() {
    for (size_t frame_id = 0; frame_id < files_.size(); frame_id++) {
        CloseFrame(frame_id);
    }
}

auto GetFilePath(table_id_t table_id) -> std::string {
    std::stringstream ss;
    ss << DATA_PATH << "/" << StringUtil::FormatFileName(table_id);
    return ss.str();
}


auto LRUTableManager::InitFile(table_id_t table_id, size_t frame_id) -> void {
    files_[frame_id] = std::make_shared<std::fstream>();
    std::string file_path = GetFilePath(table_id);
    std::error_code error_code;
    std::filesystem::create_directories(std::filesystem::path(file_path).parent_path(), error_code);
    ChickenException::AssertCondition(!error_code, "Could not create data directory: " + error_code.message());

    files_[frame_id]->open(file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!files_[frame_id]->is_open()) {
        std::ofstream create_file(file_path, std::ios::binary);
        ChickenException::AssertCondition(create_file.is_open(), "Could not create file " + file_path);
        create_file.close();
        files_[frame_id]->open(file_path, std::ios::in | std::ios::out | std::ios::binary);
    }

    ChickenException::AssertCondition(files_[frame_id]->is_open(), "Could not open file " + file_path);
    tables_[table_id] = frame_id;
    frame_table_map_[frame_id] = table_id;
}

auto LRUTableManager::CloseFrame(size_t frame_id) -> void {
    if (frame_id >= files_.size() || files_[frame_id] == nullptr) {
        return;
    }

    if (files_[frame_id]->is_open()) {
        files_[frame_id]->flush();
        files_[frame_id]->close();
    }
    files_[frame_id].reset();
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
            const auto old_table_iter = frame_table_map_.find(frame_id);
            if (old_table_iter != frame_table_map_.end()) {
                tables_.erase(old_table_iter->second);
                frame_table_map_.erase(old_table_iter);
            }
        }

        CloseFrame(frame_id);

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
}

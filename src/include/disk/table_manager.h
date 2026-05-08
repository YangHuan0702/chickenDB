//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "file_lru_manager.h"
#include "common/types.h"

namespace chickenDB {

    class LRUTableManager {
    public:
        explicit LRUTableManager(uint32_t capacity);
        ~LRUTableManager();

        auto Acquire(table_id_t table_id) -> std::shared_ptr<std::fstream>;
        auto Release(table_id_t table_id) -> void;

    private:

        auto InitFile(table_id_t,size_t) -> void;

        std::unordered_map<table_id_t, size_t> tables_{};
        std::vector<std::shared_ptr<std::fstream>> files_;
        FileLruManager file_lru_manager_;

        std::vector<size_t> free_;
        std::mutex mutex_;

    };

}

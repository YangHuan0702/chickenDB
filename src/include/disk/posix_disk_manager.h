//
// Created by 杨欢 on 2026/5/12.
//
#pragma once
#include <memory>
#include <fstream>
#include "disk_manager.h"

namespace chickenDB {

    class PosixDiskManager : public DiskManager {
    public:
        explicit PosixDiskManager(int fd);
        ~PosixDiskManager() override;

        auto ReadPage(page_id_t page_id, Page *page) -> bool override;
        auto WritePage(page_id_t page_id, Page *page) -> bool override;
        auto GetFileSize() -> size_t override;

        auto SetPageSize(size_t) -> void override;

    private:
        int fd_;
        size_t page_size_{PAGE_SIZE};
    };

}

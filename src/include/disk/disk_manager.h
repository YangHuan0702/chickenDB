//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include "buffer/page.h"
#include "common/types.h"

namespace chickenDB {

    class DiskManager {
    public:
        DiskManager() =  default;
        virtual ~DiskManager() = default;

        virtual auto ReadPage(page_id_t page_id,Page *page) -> bool = 0;

        virtual auto WritePage(page_id_t page_id,Page *page) -> bool = 0;

        virtual auto GetFileSize() -> size_t = 0;

        virtual auto SetPageSize(size_t) -> void = 0;

    };

}

//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <vector>

#include "table_catalog_entry.h"
#include "buffer/page.h"

namespace chickenDB {
    // struct TableCatalogPageHeader {
    //     uint32_t next_page_id; // 0 表示最后一页
    //     uint16_t entry_count;
    //     uint8_t reserved[10];
    //     TableCatalogEntry entries[];
    // };


    class TableCatalogPage : public Page {
    public:
        uint32_t next_page_id;
        uint16_t entry_count;
        uint64_t reserved;
        std::vector<TableCatalogEntry> entries_;
    };
}

//
// Created by huan.yang on 2026-05-07.
//

#pragma once
#include <cstdint>
#include <functional>

namespace chickenDB {

    typedef int32_t col_id_t;

    typedef uint64_t table_id_t;

    typedef uint32_t fs_id_t;

    // Local page number within a single table file (file offset = page_no * page_size).
    typedef int64_t page_id_t;

    typedef size_t frame_id_t;

    // Globally unique page identifier: which table file + which page within that file.
    struct PageId {
        table_id_t table_id{0};
        page_id_t  page_no{-1};

        bool operator==(const PageId &o) const noexcept {
            return table_id == o.table_id && page_no == o.page_no;
        }
        bool IsValid() const noexcept { return page_no >= 0; }
    };

    struct PageIdHash {
        size_t operator()(const PageId &p) const noexcept {
            size_t h1 = std::hash<table_id_t>()(p.table_id);
            size_t h2 = std::hash<page_id_t>()(p.page_no);
            return h1 ^ (h2 * 2654435761ULL);
        }
    };

    inline constexpr PageId INVALID_PAGE_ID{0, -1};

}


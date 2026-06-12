//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

#include "common/types.h"

namespace chickenDB {
    // RID（Record/Row ID）：定位一行的物理地址 = 第几个数据页 + 页内第几行。
    // 索引的 value 即 RID：通过它从 TableDataPage 取回整行。
    struct RID {
        page_id_t page_no{-1};
        uint32_t row_idx{0};

        RID() = default;
        RID(page_id_t p, uint32_t r) : page_no(p), row_idx(r) {}

        auto operator==(const RID &o) const -> bool {
            return page_no == o.page_no && row_idx == o.row_idx;
        }
        auto operator!=(const RID &o) const -> bool { return !(*this == o); }
        auto IsValid() const -> bool { return page_no >= 0; }
    };

    struct RidHash {
        auto operator()(const RID &r) const noexcept -> size_t {
            size_t h1 = std::hash<page_id_t>()(r.page_no);
            size_t h2 = std::hash<uint32_t>()(r.row_idx);
            return h1 ^ (h2 * 2654435761ULL);
        }
    };
}

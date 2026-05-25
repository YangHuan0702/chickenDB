//
// Created by huan.yang on 2026-05-25.
//
#pragma once
#include <memory>

#include "buffer_manager.h"
#include "common/types.h"

namespace chickenDB {
    class TableScanIterator {
    public:
        explicit TableScanIterator(PageId page_id, std::shared_ptr<BufferManager> buffer_manager,
                                   const size_t row_count) : page_id_(page_id),
                                                             buffer_manager_(std::move(buffer_manager)),
                                                             row_count_(row_count) {
        }

        ~TableScanIterator() = default;


        auto operator++() -> TableScanIterator &;

        auto operator==(const TableScanIterator &target) const -> bool {
            return target.page_id_ == this->page_id_ && current_page_id_ == target.current_page_id_
                   && row_count_ == target.row_count_ && is_end_ == target.is_end_;
        }

        auto operator!=(const TableScanIterator &target) const -> bool {
            return !(*this == target);
        }

        auto GetCurrentPageId() const -> page_id_t {
            return current_page_id_;
        }

    private:
        PageId page_id_;
        std::shared_ptr<BufferManager> buffer_manager_;
        page_id_t current_page_id_{0};
        size_t current_row_no_{0};
        size_t row_count_{0};
        bool is_end_{false};
    };
}

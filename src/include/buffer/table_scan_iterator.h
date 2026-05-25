//
// Created by huan.yang on 2026-05-25.
//
#pragma once
#include <memory>

#include "buffer_manager.h"
#include "catalog/schema_version.h"
#include "common/types.h"
#include "executor/chunk.h"

namespace chickenDB {
    class TableScanIterator {
    public:
        explicit TableScanIterator(table_id_t table_id, page_id_t first, page_id_t last,
                                   std::shared_ptr<BufferManager> buffer,
                                   SchemaPage *schema_page) : table_id_(table_id), first_page_id_(first),
                                                              last_page_id_(last), buffer_manager_(std::move(buffer)),
                                                              schema_page_(schema_page) {
        }
        ~TableScanIterator() = default;

        auto GetTableId() -> table_id_t { return table_id_;}
        auto GetFirstPageId() -> page_id_t { return first_page_id_;}
        auto GetLastPageId() -> page_id_t { return last_page_id_;}
        auto GetSchemaPage() -> const SchemaPage * {return schema_page_;}


        auto Next(Chunk &output) -> bool;

    private:

        auto LoadCurrentPage() -> void;
        auto AdvancePage() -> bool;
        auto FillChunkFromPage(Chunk &chunk) -> size_t;

        size_t row_offset_in_page_{0};
        Page *current_page_{nullptr};

        table_id_t table_id_;
        page_id_t first_page_id_;
        page_id_t last_page_id_;
        std::shared_ptr<BufferManager> buffer_manager_;
        const SchemaPage *schema_page_;
    };
}

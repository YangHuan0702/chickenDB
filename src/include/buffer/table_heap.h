//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <memory>

#include "buffer/buffer_manager.h"
#include "catalog/schema_version.h"
#include "common/rid.h"
#include "executor/chunk.h"

namespace chickenDB {
    // 按 RID 随机读取单行的辅助类（索引扫描回表用）。
    // RID = {page_no, row_idx}：FetchRow 把该页解码出指定行，追加到 out chunk。
    // 与 TableScanIterator 共享同一套页解码逻辑（TableDataPage + 列解压）。
    class TableHeap {
    public:
        TableHeap(table_id_t table_id, std::shared_ptr<BufferManager> buffer,
                  const SchemaPage *schema)
            : table_id_(table_id), buffer_manager_(std::move(buffer)), schema_(schema) {}

        // 把 rid 指向的行追加为 out 的下一行（out 须已 Init 为该表 schema 布局，
        // 且容量足够）。返回是否成功取到。
        auto FetchRow(const RID &rid, Chunk &out, size_t out_row) -> bool;

    private:
        table_id_t table_id_;
        std::shared_ptr<BufferManager> buffer_manager_;
        [[maybe_unused]] const SchemaPage *schema_;
    };
}

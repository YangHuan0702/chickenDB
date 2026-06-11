//
// Created by 杨欢 on 2026/5/23.
//

#include "planner/physical/scan/physical_seq_scan.h"

#include "common/chicken_execption.h"
#include "executor/executor_context.h"

using namespace chickenDB;

auto PhysicalSeqScan::Init() -> void {
    ChickenException::AssertCondition(ctx_ != nullptr, "[SeqScan] executor context not attached");
    auto buffer = ctx_->buffer_manager_;
    const SchemaPage *schema = ctx_->catalog_->GetSchema(table_id_);
    ChickenException::AssertCondition(schema != nullptr, "[SeqScan] schema not found");

    const page_id_t page_count = buffer->GetPageCount(table_id_);
    const page_id_t last = page_count > 0 ? page_count - 1 : 0;

    it_ = std::make_unique<TableScanIterator>(table_id_, /*first=*/0, /*last=*/last,
                                              buffer, const_cast<SchemaPage *>(schema));
}

auto PhysicalSeqScan::Close() -> void {
    it_.reset();
}

auto PhysicalSeqScan::Next() -> Chunk * {
    if (it_ == nullptr) {
        return nullptr;
    }
    current_chunk_.Reset();
    if (it_->Next(current_chunk_)) {
        return &current_chunk_;
    }
    return nullptr;
}

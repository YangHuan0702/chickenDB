//
// Created by 杨欢 on 2026/5/23.
//

#include "planner/physical/scan/physical_seq_scan.h"

#include "common/chicken_execption.h"
#include "executor/executor_context.h"
#include "executor/chunk_util.h"

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
    while (true) {
        current_chunk_.Reset();
        if (!it_->Next(current_chunk_)) {
            return nullptr;
        }
        // 非事务路径：直接返回整页。
        if (!ctx_->HasTxn()) {
            return &current_chunk_;
        }
        // 事务路径：按当前快照过滤可见行（RID = {当前页号, 行下标}）。
        const page_id_t page_no = it_->CurrentPageNo();
        const size_t n = current_chunk_.Count();
        visible_chunk_.Init(ChunkUtil::TypesOf(current_chunk_), n == 0 ? 1 : n);
        visible_chunk_.SetColIds(current_chunk_.ColIds());

        size_t out_row = 0;
        for (size_t r = 0; r < n; r++) {
            RID rid(page_no, static_cast<uint32_t>(r));
            if (ctx_->version_store_->IsVisible(rid, *ctx_->txn_)) {
                ChunkUtil::CopyRow(visible_chunk_, out_row, current_chunk_, r);
                out_row++;
            }
        }
        if (out_row > 0) {
            visible_chunk_.SetCount(out_row);
            return &visible_chunk_;
        }
        // 整页都不可见，继续下一页。
    }
}

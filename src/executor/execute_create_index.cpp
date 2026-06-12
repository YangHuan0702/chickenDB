//
// Created by huan.yang on 2026-06-11.
//
#include <vector>

#include "common/chicken_execption.h"
#include "executor/execution.h"
#include "executor/executor_context.h"
#include "executor/chunk_util.h"
#include "buffer/table_scan_iterator.h"
#include "planner/physical/ddl/physical_create_index.h"
#include "catalog/index_catalog_entry.h"

using namespace chickenDB;

// 注册索引到 catalog，然后全表扫描，对每行构造索引键 + RID 插入索引。
auto Execution::ExecuteCreateIndex(std::unique_ptr<PhysicalOperator> plan) -> void {
    ChickenException::AssertCondition(plan->type_ == PhysicalOperatorType::CREATE_INDEX,
                                      "[ExecuteCreateIndex] not a create index plan");
    auto *op = dynamic_cast<PhysicalCreateIndex *>(plan.get());
    ChickenException::AssertCondition(op != nullptr, "[ExecuteCreateIndex] cast failed");

    auto catalog = context_->catalog_;
    auto buffer = context_->buffer_manager_;

    // 1) 在 catalog 注册索引（创建活的 Index 实例）。
    catalog->CreateIndex(op->index_name_, op->table_id_, op->key_cols_, op->index_type_, op->unique_);
    const IndexInfo *info = catalog->GetIndex(op->index_name_);
    ChickenException::AssertCondition(info != nullptr && info->index != nullptr,
                                      "[ExecuteCreateIndex] index registration failed");

    const SchemaPage *schema = catalog->GetSchema(op->table_id_);
    ChickenException::AssertCondition(schema != nullptr, "[ExecuteCreateIndex] schema not found");

    // 2) 全表扫描填充索引。
    const page_id_t page_count = buffer->GetPageCount(op->table_id_);
    const page_id_t last = page_count > 0 ? page_count - 1 : 0;
    TableScanIterator it(op->table_id_, 0, last, buffer, const_cast<SchemaPage *>(schema));

    Chunk chunk;
    bool resolved = false;
    std::vector<size_t> key_idx;

    while (it.Next(chunk)) {
        if (!resolved) {
            auto col_map = ChunkUtil::BuildColMap(chunk);
            for (col_id_t cid : op->key_cols_) {
                auto cit = col_map.find(cid);
                ChickenException::AssertCondition(cit != col_map.end(),
                                                  "[ExecuteCreateIndex] key column not in table");
                key_idx.push_back(cit->second);
            }
            resolved = true;
        }
        const page_id_t page_no = it.CurrentPageNo();
        const size_t n = chunk.Count();
        for (size_t r = 0; r < n; r++) {
            IndexKey key(ChunkUtil::MakeIndexKey(chunk, r, key_idx));
            info->index->Insert(key, RID(page_no, static_cast<uint32_t>(r)));
        }
    }
}

//
// Created by huan.yang on 2026-06-11.
//
// 三个基于索引的扫描算子的执行实现：
//   IndexScan / BitmapScan -> 绑定索引时走点查/范围查 + 回表，否则全表扫描 + 谓词过滤
//   IndexOnlyScan          -> 绑定覆盖索引时直接从索引键产出列（不回表），否则全扫 + 投影
//
#include "planner/physical/scan/physical_index_scan.h"
#include "planner/physical/scan/physical_index_olny_scan.h"
#include "planner/physical/scan/physical_bitmap_scan.h"

#include "common/chicken_execption.h"
#include "executor/executor_context.h"
#include "executor/expression_evaluator.h"
#include "executor/chunk_util.h"

using namespace chickenDB;

namespace {
    // 构造一个覆盖该表全部数据页的扫描迭代器（与 SeqScan 一致）。
    auto MakeScanIterator(ExecutorContext *ctx, table_id_t table_id)
        -> std::unique_ptr<TableScanIterator> {
        ChickenException::AssertCondition(ctx != nullptr, "[Scan] executor context not attached");
        auto buffer = ctx->buffer_manager_;
        const SchemaPage *schema = ctx->catalog_->GetSchema(table_id);
        ChickenException::AssertCondition(schema != nullptr, "[Scan] schema not found");
        const page_id_t page_count = buffer->GetPageCount(table_id);
        const page_id_t last = page_count > 0 ? page_count - 1 : 0;
        return std::make_unique<TableScanIterator>(table_id, 0, last, buffer,
                                                   const_cast<SchemaPage *>(schema));
    }

    // 全扫 + 谓词：从迭代器拉一批，对每行求 predicate，命中行物化进 output。
    // predicate 为空表示全部命中。返回 false 表示扫描结束。
    auto ScanFilterNext(TableScanIterator *it, Chunk &scan_chunk, Chunk &output,
                        const BoundExpression *predicate,
                        std::unordered_map<col_id_t, size_t> &col_map) -> bool {
        while (true) {
            scan_chunk.Reset();
            if (!it->Next(scan_chunk)) {
                return false;
            }
            if (col_map.empty()) {
                col_map = ChunkUtil::BuildColMap(scan_chunk);
            }
            const size_t n = scan_chunk.Count();
            output.Init(ChunkUtil::TypesOf(scan_chunk), n == 0 ? 1 : n);
            output.SetColIds(scan_chunk.ColIds());

            size_t out_row = 0;
            for (size_t r = 0; r < n; r++) {
                if (predicate == nullptr ||
                    ExpressionEvaluator::EvalPredicate(predicate, scan_chunk, r, col_map)) {
                    ChunkUtil::CopyRow(output, out_row, scan_chunk, r);
                    out_row++;
                }
            }
            if (out_row > 0) {
                output.SetCount(out_row);
                return true;
            }
            // 整批被过滤，继续下一批。
        }
    }
}

// ---- IndexScan：有索引走索引点查/范围查 + 回表；否则退化全扫 + 过滤 ----
auto PhysicalIndexScan::Init() -> void {
    use_index_ = false;
    emitted_ = false;
    rids_.clear();
    col_map_.clear();

    const SchemaPage *schema = ctx_->catalog_->GetSchema(table_id_);
    ChickenException::AssertCondition(schema != nullptr, "[IndexScan] schema not found");

    // 尝试走索引路径。
    if (!index_name_.empty()) {
        const IndexInfo *info = ctx_->catalog_->GetIndex(index_name_);
        if (info != nullptr && info->index != nullptr) {
            if (is_range_) {
                ChickenException::AssertCondition(info->index->SupportsRange(),
                                                  "[IndexScan] index does not support range");
                rids_ = info->index->Range(lookup_lo_, lookup_hi_);
            } else {
                rids_ = info->index->Find(lookup_lo_);
            }
            heap_ = std::make_unique<TableHeap>(table_id_, ctx_->buffer_manager_, schema);
            use_index_ = true;
            return;
        }
    }
    // 回退：全表扫描。
    it_ = MakeScanIterator(ctx_, table_id_);
}

auto PhysicalIndexScan::Close() -> void {
    it_.reset();
    heap_.reset();
}

auto PhysicalIndexScan::Next() -> Chunk * {
    if (use_index_) {
        if (emitted_) return nullptr;
        emitted_ = true;

        const SchemaPage *schema = ctx_->catalog_->GetSchema(table_id_);
        std::vector<ColumnType> types;
        std::vector<col_id_t> ids;
        for (const auto &col : schema->columns_) {
            types.push_back(col.data_type);
            ids.push_back(col.col_id);
        }
        output_.Init(types, rids_.empty() ? 1 : rids_.size());
        output_.SetColIds(ids);

        size_t out_row = 0;
        for (const RID &rid : rids_) {
            // 事务路径：仅取当前快照可见的行。
            if (ctx_->HasTxn() && !ctx_->version_store_->IsVisible(rid, *ctx_->txn_)) {
                continue;
            }
            if (heap_->FetchRow(rid, output_, out_row)) {
                out_row++;
            }
        }
        output_.SetCount(out_row);
        return out_row > 0 ? &output_ : nullptr;
    }

    // 全扫回退。
    if (it_ == nullptr) return nullptr;
    return ScanFilterNext(it_.get(), scan_chunk_, output_, predicate_.get(), col_map_)
               ? &output_ : nullptr;
}

// ---- BitmapScan：走 bitmap 索引点查/范围得 RID 回表；否则退化全扫 + 过滤 ----
auto PhysicalBitmapScan::Init() -> void {
    use_index_ = false;
    emitted_ = false;
    rids_.clear();
    col_map_.clear();

    const SchemaPage *schema = ctx_->catalog_->GetSchema(table_id_);
    ChickenException::AssertCondition(schema != nullptr, "[BitmapScan] schema not found");

    if (!index_name_.empty()) {
        const IndexInfo *info = ctx_->catalog_->GetIndex(index_name_);
        if (info != nullptr && info->index != nullptr) {
            if (is_range_) {
                rids_ = info->index->Range(lookup_lo_, lookup_hi_);
            } else {
                rids_ = info->index->Find(lookup_lo_);
            }
            heap_ = std::make_unique<TableHeap>(table_id_, ctx_->buffer_manager_, schema);
            use_index_ = true;
            return;
        }
    }
    it_ = MakeScanIterator(ctx_, table_id_);
}

auto PhysicalBitmapScan::Close() -> void {
    it_.reset();
    heap_.reset();
}

auto PhysicalBitmapScan::Next() -> Chunk * {
    if (use_index_) {
        if (emitted_) return nullptr;
        emitted_ = true;
        const SchemaPage *schema = ctx_->catalog_->GetSchema(table_id_);
        std::vector<ColumnType> types;
        std::vector<col_id_t> ids;
        for (const auto &col : schema->columns_) { types.push_back(col.data_type); ids.push_back(col.col_id); }
        output_.Init(types, rids_.empty() ? 1 : rids_.size());
        output_.SetColIds(ids);
        size_t out_row = 0;
        for (const RID &rid : rids_) {
            if (ctx_->HasTxn() && !ctx_->version_store_->IsVisible(rid, *ctx_->txn_)) continue;
            if (heap_->FetchRow(rid, output_, out_row)) out_row++;
        }
        output_.SetCount(out_row);
        return out_row > 0 ? &output_ : nullptr;
    }
    if (it_ == nullptr) return nullptr;
    return ScanFilterNext(it_.get(), scan_chunk_, output_, predicate_.get(), col_map_)
               ? &output_ : nullptr;
}

// ---- IndexOnlyScan：从索引直接产出覆盖列（不回表）；否则退化全扫 + 投影 ----
auto PhysicalIndexOnlyScan::Init() -> void {
    use_index_ = false;
    emitted_ = false;
    src_idx_.clear();
    if (!index_name_.empty() && ctx_->catalog_->GetIndex(index_name_) != nullptr) {
        use_index_ = true;
        return;
    }
    it_ = MakeScanIterator(ctx_, table_id_);
}

auto PhysicalIndexOnlyScan::Close() -> void { it_.reset(); }

auto PhysicalIndexOnlyScan::Next() -> Chunk * {
    if (use_index_) {
        if (emitted_) return nullptr;
        emitted_ = true;

        const IndexInfo *info = ctx_->catalog_->GetIndex(index_name_);
        const auto &key_cols = info->key_cols; // 索引键列顺序即 ScanAll 键的分量顺序
        // 输出列 = output_cols_（空则取全部键列）。建立输出列 -> 键分量下标。
        std::vector<col_id_t> out_ids = output_cols_.empty() ? key_cols : output_cols_;
        std::vector<size_t> key_pos; // out_ids[i] 在 key_cols 中的下标
        for (col_id_t oc : out_ids) {
            size_t p = 0;
            for (; p < key_cols.size(); p++) if (key_cols[p] == oc) break;
            ChickenException::AssertCondition(p < key_cols.size(),
                                              "[IndexOnlyScan] output column not covered by index");
            key_pos.push_back(p);
        }

        // 列类型：从 schema 取。
        const SchemaPage *schema = ctx_->catalog_->GetSchema(table_id_);
        std::vector<ColumnType> out_types;
        for (col_id_t oc : out_ids) {
            for (const auto &col : schema->columns_) {
                if (col.col_id == oc) { out_types.push_back(col.data_type); break; }
            }
        }

        auto entries = info->index->ScanAll();
        output_.Init(out_types, entries.empty() ? 1 : entries.size());
        output_.SetColIds(out_ids);
        size_t out_row = 0;
        for (const auto &kv : entries) {
            if (ctx_->HasTxn() && !ctx_->version_store_->IsVisible(kv.second, *ctx_->txn_)) continue;
            for (size_t c = 0; c < out_ids.size(); c++) {
                const double v = kv.first.vals[key_pos[c]];
                if (out_types[c] == ColumnType::NUMBER) {
                    output_.GetColumn(c).SetValue<int32_t>(out_row, static_cast<int32_t>(v));
                } else {
                    output_.GetColumn(c).SetValue<double>(out_row, v);
                }
            }
            out_row++;
        }
        output_.SetCount(out_row);
        return out_row > 0 ? &output_ : nullptr;
    }

    // 全扫回退（投影覆盖列）。
    if (it_ == nullptr) return nullptr;
    scan_chunk_.Reset();
    if (!it_->Next(scan_chunk_)) {
        return nullptr;
    }
    if (src_idx_.empty()) {
        if (output_cols_.empty()) {
            for (size_t i = 0; i < scan_chunk_.ColumnCount(); i++) src_idx_.push_back(i);
        } else {
            auto col_map = ChunkUtil::BuildColMap(scan_chunk_);
            for (col_id_t cid : output_cols_) {
                auto it = col_map.find(cid);
                ChickenException::AssertCondition(it != col_map.end(),
                                                  "[IndexOnlyScan] covering column not found");
                src_idx_.push_back(it->second);
            }
        }
    }
    const size_t n = scan_chunk_.Count();
    std::vector<ColumnType> out_types;
    std::vector<col_id_t> out_ids;
    out_types.reserve(src_idx_.size());
    out_ids.reserve(src_idx_.size());
    for (size_t idx : src_idx_) {
        out_types.push_back(scan_chunk_.GetColumn(idx).GetType());
        out_ids.push_back(scan_chunk_.ColIds()[idx]);
    }
    output_.Init(out_types, n == 0 ? 1 : n);
    output_.SetColIds(out_ids);
    for (size_t r = 0; r < n; r++) {
        ChunkUtil::CopyRowProjected(output_, r, scan_chunk_, r, src_idx_);
    }
    output_.SetCount(n);
    return &output_;
}

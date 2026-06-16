//
// Created by huan.yang on 2026-06-11.
//
#include <cstring>
#include <unordered_map>
#include <vector>

#include "common/chicken_execption.h"
#include "executor/execution.h"
#include "executor/executor_context.h"
#include "executor/chunk_util.h"
#include "executor/expression_evaluator.h"
#include "buffer/table_scan_iterator.h"
#include "buffer/table_data_page.h"
#include "planner/physical/dml/physical_delete.h"
#include "planner/physical/dml/physical_update.h"
#include "transaction/log_record.h"

using namespace chickenDB;

namespace {
    // 记一条 MVCC 删除：version store OnDelete + WAL + 事务 delete set。
    auto MarkDelete(ExecutorContext *ctx, table_id_t table_id, const RID &rid) -> void {
        if (!ctx->HasTxn()) return;
        const txn_id_t tid = ctx->txn_->GetTxnId();
        if (ctx->log_manager_ != nullptr) {
            LogRecord rec;
            rec.type = LogRecordType::DELETE;
            rec.txn_id = tid;
            rec.table_id = table_id;
            rec.rid_page = rid.page_no;
            rec.rid_row = rid.row_idx;
            ctx->log_manager_->Append(rec);
        }
        ctx->version_store_->OnDelete(rid, tid);
        ctx->txn_->AppendDelete(rid);
    }
}

// 扫表按谓词匹配行，对命中行（仅当前快照可见的）做 MVCC 删除。
auto Execution::ExecuteDelete(std::unique_ptr<PhysicalOperator> plan) -> void {
    auto *op = dynamic_cast<PhysicalDelete *>(plan.get());
    ChickenException::AssertCondition(op != nullptr, "[ExecuteDelete] cast failed");

    auto catalog = context_->catalog_;
    auto buffer = context_->buffer_manager_;
    const SchemaPage *schema = catalog->GetSchema(op->table_id_);
    ChickenException::AssertCondition(schema != nullptr, "[ExecuteDelete] schema not found");

    const page_id_t page_count = buffer->GetPageCount(op->table_id_);
    const page_id_t last = page_count > 0 ? page_count - 1 : 0;
    TableScanIterator it(op->table_id_, 0, last, buffer, const_cast<SchemaPage *>(schema));

    Chunk chunk;
    std::unordered_map<col_id_t, size_t> col_map;
    while (it.Next(chunk)) {
        if (col_map.empty()) col_map = ChunkUtil::BuildColMap(chunk);
        const page_id_t page_no = it.CurrentPageNo();
        const size_t n = chunk.Count();
        for (size_t r = 0; r < n; r++) {
            RID rid(page_no, static_cast<uint32_t>(r));
            // 只删除当前事务可见的行。
            if (context_->HasTxn() && !context_->version_store_->IsVisible(rid, *context_->txn_)) {
                continue;
            }
            if (op->predicate_ == nullptr ||
                ExpressionEvaluator::EvalPredicate(op->predicate_.get(), chunk, r, col_map)) {
                MarkDelete(context_.get(), op->table_id_, rid);
                // 索引维护：从该表活索引删除此行的键。
                auto col_value = [&](col_id_t cid) -> IndexKeyVal {
                    auto cit = col_map.find(cid);
                    if (cit == col_map.end()) return IndexKeyVal(0.0);
                    const Vector &v = chunk.GetColumn(cit->second);
                    if (v.IsVar()) return IndexKeyVal(std::string(v.GetString(r)));
                    return IndexKeyVal(v.GetType() == ColumnType::NUMBER
                                           ? static_cast<double>(v.GetValue<int32_t>(r))
                                           : v.GetValue<double>(r));
                };
                context_->catalog_->MaintainIndexDelete(op->table_id_, col_value, rid);
            }
        }
    }
}

// UPDATE = 对命中行删旧 + 插新（新行 = 旧行 + col_ids/values 覆盖）。
auto Execution::ExecuteUpdate(std::unique_ptr<PhysicalOperator> plan) -> void {
    auto *op = dynamic_cast<PhysicalUpdate *>(plan.get());
    ChickenException::AssertCondition(op != nullptr, "[ExecuteUpdate] cast failed");

    auto catalog = context_->catalog_;
    auto buffer = context_->buffer_manager_;
    const SchemaPage *schema = catalog->GetSchema(op->table_id_);
    ChickenException::AssertCondition(schema != nullptr, "[ExecuteUpdate] schema not found");

    // UPDATE 当前把行物化为 double（旧行/新行），不支持变长列；含 VARCHAR 列报错。
    // （DELETE 已支持变长；UPDATE 的变长支持留待后续。）
    for (const auto &col : schema->columns_) {
        ChickenException::AssertCondition(!IsVarlen(col.data_type),
            "[ExecuteUpdate] update on table with varchar column not supported yet");
    }

    // col_id -> 新值 映射。
    std::unordered_map<col_id_t, const Value *> set_map;
    for (size_t i = 0; i < op->col_ids_.size(); i++) {
        set_map[op->col_ids_[i]] = &op->values_[i];
    }

    const page_id_t page_count = buffer->GetPageCount(op->table_id_);
    const page_id_t last = page_count > 0 ? page_count - 1 : 0;
    TableScanIterator it(op->table_id_, 0, last, buffer, const_cast<SchemaPage *>(schema));

    // 收集要插入的新行（扫描期间不就地改页，避免迭代器失效）。
    std::vector<std::vector<double>> new_rows;
    std::vector<std::vector<double>> old_rows; // 与 to_delete 并列，用于索引删除维护
    std::vector<RID> to_delete;

    Chunk chunk;
    std::unordered_map<col_id_t, size_t> col_map;
    while (it.Next(chunk)) {
        if (col_map.empty()) col_map = ChunkUtil::BuildColMap(chunk);
        const page_id_t page_no = it.CurrentPageNo();
        const size_t n = chunk.Count();
        for (size_t r = 0; r < n; r++) {
            RID rid(page_no, static_cast<uint32_t>(r));
            if (context_->HasTxn() && !context_->version_store_->IsVisible(rid, *context_->txn_)) {
                continue;
            }
            if (op->predicate_ != nullptr &&
                !ExpressionEvaluator::EvalPredicate(op->predicate_.get(), chunk, r, col_map)) {
                continue;
            }
            // 旧行各列值（用于索引删除）。
            std::vector<double> old_row;
            old_row.reserve(schema->columns_.size());
            for (size_t c = 0; c < schema->columns_.size(); c++) {
                const Vector &vec = chunk.GetColumn(c);
                old_row.push_back(vec.GetType() == ColumnType::NUMBER
                                      ? static_cast<double>(vec.GetValue<int32_t>(r))
                                      : vec.GetValue<double>(r));
            }
            // 构造新行：各列取旧值，col_ids 指定列替换为新值（统一 double 表示）。
            std::vector<double> row = old_row;
            for (size_t c = 0; c < schema->columns_.size(); c++) {
                const auto &col = schema->columns_[c];
                auto sit = set_map.find(col.col_id);
                if (sit != set_map.end()) {
                    const Value &v = *sit->second;
                    double dv = 0;
                    if (std::holds_alternative<int64_t>(v.value_)) dv = static_cast<double>(std::get<int64_t>(v.value_));
                    else if (std::holds_alternative<int>(v.value_)) dv = static_cast<double>(std::get<int>(v.value_));
                    else if (std::holds_alternative<double>(v.value_)) dv = std::get<double>(v.value_);
                    else if (std::holds_alternative<float>(v.value_)) dv = static_cast<double>(std::get<float>(v.value_));
                    row[c] = dv;
                }
            }
            new_rows.push_back(std::move(row));
            old_rows.push_back(std::move(old_row));
            to_delete.push_back(rid);
        }
    }

    // 列 col_id -> schema 下标，供索引维护按列取值。
    auto col_idx_of = [&](col_id_t cid) -> size_t {
        for (size_t c = 0; c < schema->columns_.size(); c++) {
            if (schema->columns_[c].col_id == cid) return c;
        }
        return 0;
    };

    // 删旧行 + 索引删除维护。
    for (size_t i = 0; i < to_delete.size(); i++) {
        MarkDelete(context_.get(), op->table_id_, to_delete[i]);
        const std::vector<double> &orow = old_rows[i];
        auto col_value = [&](col_id_t cid) -> IndexKeyVal { return IndexKeyVal(orow[col_idx_of(cid)]); };
        context_->catalog_->MaintainIndexDelete(op->table_id_, col_value, to_delete[i]);
    }

    // 插新行：每行单独建页（与 INSERT 单页假设一致），登记版本 + WAL。
    const txn_id_t tid = context_->HasTxn() ? context_->txn_->GetTxnId() : INVALID_TXN_ID;
    for (const auto &row : new_rows) {
        const size_t ncols = schema->columns_.size();
        std::vector<std::vector<char>> col_buffers(ncols);
        std::vector<std::vector<uint8_t>> col_validity(ncols);
        std::vector<ColumnInput> inputs;
        inputs.reserve(ncols);
        for (size_t c = 0; c < ncols; c++) {
            const auto &col = schema->columns_[c];
            const size_t ts = TypeSizeConversion::TypeSize(col.data_type);
            col_buffers[c].resize(ts, 0);
            col_validity[c].assign(1, 0xFF);
            if (col.data_type == ColumnType::NUMBER) {
                int32_t iv = static_cast<int32_t>(row[c]);
                std::memcpy(col_buffers[c].data(), &iv, sizeof(int32_t));
            } else {
                double dv = row[c];
                std::memcpy(col_buffers[c].data(), &dv, sizeof(double));
            }
            inputs.push_back(ColumnInput{col.col_id, col.data_type, col_buffers[c].data(),
                                         static_cast<uint32_t>(ts), col_validity[c].data()});
        }

        Page *page = buffer->NewPage(op->table_id_);
        const page_id_t page_no = page->page_id_.page_no;
        TableDataPage data_page(op->table_id_, page);
        bool ok = data_page.BuildFromColumns(inputs, 1, CompressionType::ZSTD, 0, 0);
        buffer->UnpinPage(op->table_id_, page_no, true);
        ChickenException::AssertCondition(ok, "[ExecuteUpdate] new row does not fit page");

        if (context_->HasTxn()) {
            RID rid(page_no, 0);
            if (context_->log_manager_ != nullptr) {
                LogRecord rec;
                rec.type = LogRecordType::INSERT;
                rec.txn_id = tid;
                rec.table_id = op->table_id_;
                rec.rid_page = page_no;
                rec.rid_row = 0;
                context_->log_manager_->Append(rec);
            }
            context_->version_store_->OnInsert(rid, tid);
            context_->txn_->AppendInsert(rid);
        }
        // 索引维护：新行键插入。
        {
            RID rid(page_no, 0);
            auto col_value = [&](col_id_t cid) -> IndexKeyVal { return IndexKeyVal(row[col_idx_of(cid)]); };
            context_->catalog_->MaintainIndexInsert(op->table_id_, col_value, rid);
        }
        context_->catalog_->AddRowCount(op->table_id_, 1);
    }

    if (context_->HasTxn() && context_->log_manager_ != nullptr) {
        context_->log_manager_->Flush();
    }
}

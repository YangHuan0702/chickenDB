//
// Created by huan.yang on 2026-06-11.
//
#include "executor/session.h"

#include "binder/binder.h"
#include "binder/expression/bound_column_expression.h"
#include "binder/statement/bound_select_statement.h"
#include "common/chicken_execption.h"
#include "executor/execution.h"
#include "executor/executor_context.h"
#include "optimizer/optimizer.h"
#include "parser/parser.h"
#include "parser/statment/transaction_statement.h"
#include "planner/planner.h"
#include "transaction/log_record.h"

using namespace chickenDB;

namespace {
    auto ColumnNameFor(const std::shared_ptr<Catalog> &catalog, table_id_t table_id, col_id_t col_id) -> std::string {
        const SchemaPage *schema = catalog->GetSchema(table_id);
        if (schema == nullptr) return "";
        for (const auto &col : schema->columns_) {
            if (col.col_id == col_id) return col.GetColumnName();
        }
        return "";
    }

    auto ResultColumnNames(const std::shared_ptr<Catalog> &catalog, const BoundStatement *statement)
        -> std::vector<std::string> {
        std::vector<std::string> names;
        if (statement == nullptr || statement->type_ != StatementType::SELECT) return names;

        const auto *select = dynamic_cast<const BoundSelectStatement *>(statement);
        if (select == nullptr) return names;

        for (const auto &expr : select->columns_) {
            const auto *col = dynamic_cast<const BoundColumnExpression *>(expr.get());
            if (col == nullptr) {
                names.emplace_back("");
                continue;
            }

            std::string name = ColumnNameFor(catalog, col->table_id_, col->col_id_);
            if (col->is_aggregate_ && !col->agg_func_.empty()) {
                name = col->agg_func_ + "(" + name + ")";
            }
            names.push_back(name.empty() ? "column" : name);
        }
        return names;
    }
}

Session::Session(std::shared_ptr<BufferManager> buffer, std::shared_ptr<Catalog> catalog,
                 std::shared_ptr<TransactionManager> txn_manager,
                 std::shared_ptr<VersionStore> version_store,
                 std::shared_ptr<LogManager> log_manager)
    : buffer_(std::move(buffer)), catalog_(std::move(catalog)),
      txn_manager_(std::move(txn_manager)), version_store_(std::move(version_store)),
      log_manager_(std::move(log_manager)) {}

auto Session::BeginTxn() -> void {
    current_txn_ = txn_manager_->Begin();
    if (log_manager_ != nullptr) {
        LogRecord rec;
        rec.type = LogRecordType::BEGIN;
        rec.txn_id = current_txn_->GetTxnId();
        log_manager_->Append(rec);
    }
}

auto Session::FinalizeCommit(const std::shared_ptr<Transaction> &txn) -> void {
    // 先写 COMMIT 日志并 flush（write-ahead：提交点持久化后才让版本可见）。
    if (log_manager_ != nullptr) {
        LogRecord rec;
        rec.type = LogRecordType::COMMIT;
        rec.txn_id = txn->GetTxnId();
        rec.commit_ts = txn->GetCommitTs(); // 由 TransactionManager::Commit 先分配
        log_manager_->Append(rec);
        log_manager_->Flush();
    }
    // 回填版本时间戳，使本事务的写对后续快照可见。
    const timestamp_t cts = txn->GetCommitTs();
    for (const RID &rid : txn->InsertSet()) {
        version_store_->CommitInsert(rid, cts);
    }
    for (const RID &rid : txn->DeleteSet()) {
        version_store_->CommitDelete(rid, cts);
    }
}

auto Session::CommitTxn() -> void {
    if (current_txn_ == nullptr) return;
    // 先让管理器分配 commit_ts 并标记，再回填版本（FinalizeCommit 读 commit_ts）。
    txn_manager_->Commit(current_txn_);
    FinalizeCommit(current_txn_);
    current_txn_ = nullptr;
    in_explicit_txn_ = false;
}

auto Session::AbortTxn() -> void {
    if (current_txn_ == nullptr) return;
    if (log_manager_ != nullptr) {
        LogRecord rec;
        rec.type = LogRecordType::ABORT;
        rec.txn_id = current_txn_->GetTxnId();
        log_manager_->Append(rec);
        log_manager_->Flush();
    }
    // 撤销本事务的版本痕迹。
    for (const RID &rid : current_txn_->InsertSet()) {
        version_store_->AbortInsert(rid);
    }
    for (const RID &rid : current_txn_->DeleteSet()) {
        version_store_->AbortDelete(rid);
    }
    txn_manager_->Abort(current_txn_);
    current_txn_ = nullptr;
    in_explicit_txn_ = false;
}

auto Session::Execute(const std::string &sql) -> void {
    last_result_.clear();
    last_column_names_.clear();

    Parser parser;
    parser.ParserQuery(sql);

    Planner planner;
    planner.SetCatalog(catalog_);
    for (auto &stmt : parser.statements_) {
        // 事务控制语句：在绑定前直接作用于会话（不进入算子流水线）。
        if (stmt->type_ == StatementType::TRANSACTION) {
            auto *ts = dynamic_cast<TransactionStatement *>(stmt.get());
            switch (ts->command_) {
                case TransactionCommand::BEGIN:
                    ChickenException::AssertCondition(!in_explicit_txn_, "[Session] already in transaction");
                    BeginTxn();
                    in_explicit_txn_ = true;
                    break;
                case TransactionCommand::COMMIT:
                    CommitTxn();
                    break;
                case TransactionCommand::ROLLBACK:
                    AbortTxn();
                    break;
            }
            continue;
        }

        // 自动提交：无显式事务时为本语句开一个事务。
        const bool autocommit = !in_explicit_txn_;
        if (current_txn_ == nullptr) {
            BeginTxn();
        }

        // 单条语句过 binder + planner + executor。
        Binder binder(catalog_);
        auto bound = binder.BinderStatement(std::move(stmt));
        last_column_names_ = ResultColumnNames(catalog_, bound.get());

        auto ctx = std::make_unique<ExecutorContext>(buffer_, catalog_);
        ctx->txn_manager_ = txn_manager_;
        ctx->version_store_ = version_store_;
        ctx->log_manager_ = log_manager_;
        ctx->txn_ = current_txn_;

        auto logical = planner.CreateLogicalPlanner(std::move(bound));

        // 在逻辑计划和物理计划之间应用优化器。
        Optimizer optimizer(catalog_);
        logical = optimizer.Optimize(std::move(logical));

        auto physical = planner.CreatePhysicalPlanner(std::move(logical));

        Execution exec(std::move(ctx));
        exec.Exec(std::move(physical));
        last_result_ = exec.result_rows_;

        if (autocommit) {
            CommitTxn();
        }
    }
}

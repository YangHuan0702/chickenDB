//
// Created by huan.yang on 2026-06-30.
//
#include "optimizer/rule/predicate_pushdown.h"

#include <algorithm>

#include "binder/expression/bound_binary_expression.h"
#include "binder/expression/bound_column_expression.h"
#include "common/enum/binary_opt_type.h"
#include "planner/logical/logical_filter.h"
#include "planner/logical/logical_join.h"
#include "planner/logical/logical_project.h"
#include "planner/logical/logical_scan.h"

using namespace chickenDB;

// ---------- 静态工具 ----------

auto PredicatePushdown::SplitAnd(std::unique_ptr<BoundExpression> expr,
                                  std::vector<std::unique_ptr<BoundExpression>> &out) -> void {
    if (expr == nullptr) return;

    if (expr->type_ == BinderExpressionType::BINARY_OP) {
        auto *bin = static_cast<BoundBinaryExpression *>(expr.get());
        if (bin->type_ == BinaryOpExpressionType::AND) {
            // 先释放所有权再递归，避免通过 bin 访问已移走的子节点。
            auto *raw = static_cast<BoundBinaryExpression *>(expr.release());
            std::unique_ptr<BoundBinaryExpression> owned(raw);
            SplitAnd(std::move(owned->left_),  out);
            SplitAnd(std::move(owned->right_), out);
            return;
        }
    }
    out.push_back(std::move(expr));
}

auto PredicatePushdown::CombineAnd(std::vector<std::unique_ptr<BoundExpression>> preds)
    -> std::unique_ptr<BoundExpression> {
    if (preds.empty()) return nullptr;

    // 从右向左折叠成左深树：((p0 AND p1) AND p2) ...
    auto result = std::move(preds[0]);
    for (size_t i = 1; i < preds.size(); ++i) {
        auto combined = std::make_unique<BoundBinaryExpression>(BinaryOpExpressionType::AND);
        combined->left_  = std::move(result);
        combined->right_ = std::move(preds[i]);
        result = std::move(combined);
    }
    return result;
}

auto PredicatePushdown::CollectTableRefs(const BoundExpression *expr,
                                          std::vector<table_id_t> &out) -> void {
    if (expr == nullptr) return;

    if (expr->type_ == BinderExpressionType::COLUMN) {
        const auto *col = static_cast<const BoundColumnExpression *>(expr);
        // 避免重复
        if (std::find(out.begin(), out.end(), col->table_id_) == out.end()) {
            out.push_back(col->table_id_);
        }
        return;
    }

    if (expr->type_ == BinderExpressionType::BINARY_OP) {
        const auto *bin = static_cast<const BoundBinaryExpression *>(expr);
        CollectTableRefs(bin->left_.get(),  out);
        CollectTableRefs(bin->right_.get(), out);
    }
}

auto PredicatePushdown::OnlyRefs(const BoundExpression *expr,
                                  const std::vector<table_id_t> &allowed) -> bool {
    std::vector<table_id_t> refs;
    CollectTableRefs(expr, refs);
    for (table_id_t t : refs) {
        if (std::find(allowed.begin(), allowed.end(), t) == allowed.end()) {
            return false;
        }
    }
    return true;
}

// ---------- 核心下推逻辑 ----------

auto PredicatePushdown::WrapWithFilter(std::unique_ptr<LogicalOperator> plan,
                                        std::vector<std::unique_ptr<BoundExpression>> preds)
    -> std::unique_ptr<LogicalOperator> {
    if (preds.empty()) return plan;

    auto combined = CombineAnd(std::move(preds));
    auto filter   = std::make_unique<LogicalFilter>(std::move(combined));
    filter->children_.push_back(std::move(plan));
    return filter;
}

// 收集子树所有 Scan 节点的 table_id（用于确定 Join 两侧的表集合）。
namespace {
    auto CollectScanTables(const LogicalOperator *node, std::vector<table_id_t> &out) -> void {
        if (node == nullptr) return;
        if (node->type_ == LogicalOperatorType::SCAN) {
            const auto *scan = static_cast<const LogicalScan *>(node);
            out.push_back(scan->table_id_);
        }
        for (const auto &c : node->children_) {
            CollectScanTables(c.get(), out);
        }
    }
} // namespace

auto PredicatePushdown::PushdownPlan(std::unique_ptr<LogicalOperator> plan,
                                      std::vector<std::unique_ptr<BoundExpression>> extra_preds)
    -> std::unique_ptr<LogicalOperator> {
    if (plan == nullptr) {
        return WrapWithFilter(std::move(plan), std::move(extra_preds));
    }

    switch (plan->type_) {

        // ── Filter ──────────────────────────────────────────────────────────
        case LogicalOperatorType::FILTER: {
            auto *filter = static_cast<LogicalFilter *>(plan.get());

            // 把本节点的谓词拆开，与额外谓词合并一起下推。
            SplitAnd(std::move(filter->condition_), extra_preds);

            // 取走唯一子节点，把 plan 销毁（条件已移走，不再需要这层 Filter）。
            std::unique_ptr<LogicalOperator> child;
            if (!plan->children_.empty()) {
                child = std::move(plan->children_[0]);
            }
            return PushdownPlan(std::move(child), std::move(extra_preds));
        }

        // ── Project ─────────────────────────────────────────────────────────
        // Filter → Project → X  ⟹  Project → Filter → X
        // 谓词可以直接穿越 Project（Project 不改变行数，只改变列）。
        case LogicalOperatorType::PROJECT: {
            std::unique_ptr<LogicalOperator> child;
            if (!plan->children_.empty()) {
                child = std::move(plan->children_[0]);
                plan->children_.clear();
            }
            // 先把谓词推进子树。
            auto new_child = PushdownPlan(std::move(child), std::move(extra_preds));
            plan->children_.push_back(std::move(new_child));
            return plan;
        }

        // ── Join ─────────────────────────────────────────────────────────────
        case LogicalOperatorType::JOIN: {
            auto *join = static_cast<LogicalJoin *>(plan.get());

            // 收集左右子树各自包含的 table_id。
            std::vector<table_id_t> left_tables, right_tables;
            if (join->left_)  CollectScanTables(join->left_.get(),  left_tables);
            if (join->right_) CollectScanTables(join->right_.get(), right_tables);

            // 也要把 join->left_ / right_ 从 children_ 中找出来（两种存储方式均支持）。
            // LogicalJoin 把左右子节点同时存在 left_/right_ 和 children_ 里；
            // 这里统一以 children_[0]/[1] 操作，left_/right_ 保持同步。
            //（见 planner 的 LogicalSelectPlanner 实现）

            std::vector<std::unique_ptr<BoundExpression>> left_preds, right_preds, remain_preds;

            for (auto &pred : extra_preds) {
                if (OnlyRefs(pred.get(), left_tables)) {
                    left_preds.push_back(std::move(pred));
                } else if (OnlyRefs(pred.get(), right_tables)) {
                    right_preds.push_back(std::move(pred));
                } else {
                    remain_preds.push_back(std::move(pred));
                }
            }

            // 递归处理左右子树（各自不带 extra_preds，下推各自的谓词）。
            if (plan->children_.size() >= 2) {
                plan->children_[0] = PushdownPlan(
                    std::move(plan->children_[0]), std::move(left_preds));
                plan->children_[1] = PushdownPlan(
                    std::move(plan->children_[1]), std::move(right_preds));
            }

            // 跨侧谓词留在 Join 上方。
            return WrapWithFilter(std::move(plan), std::move(remain_preds));
        }

        // ── Scan ──────────────────────────────────────────────────────────────
        // 谓词到达叶子，直接包一层 Filter（无法继续下推）。
        case LogicalOperatorType::SCAN: {
            return WrapWithFilter(std::move(plan), std::move(extra_preds));
        }

        // ── 其他节点（Aggregate / Sort / Limit 等）─────────────────────────
        // 谓词不穿越聚合或排序（会改变语义），留在当前节点上方。
        default: {
            // 先递归处理子节点（不传 extra_preds）。
            for (auto &child : plan->children_) {
                child = PushdownPlan(std::move(child), {});
            }
            // 剩余谓词留在原位。
            return WrapWithFilter(std::move(plan), std::move(extra_preds));
        }
    }
}

auto PredicatePushdown::Apply(std::unique_ptr<LogicalOperator> plan)
    -> std::unique_ptr<LogicalOperator> {
    return PushdownPlan(std::move(plan), {});
}

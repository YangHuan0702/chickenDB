//
// Created by huan.yang on 2026-05-11.
//
#include "binder/statement/bound_select_statement.h"
#include "binder/expression/bound_binary_expression.h"
#include "binder/expression/bound_column_expression.h"
#include "binder/expression/bound_constant_expression.h"
#include "common/chicken_execption.h"
#include "planner/planner.h"
#include "planner/logical/logical_filter.h"
#include "planner/logical/logical_project.h"
#include "planner/logical/logical_scan.h"
#include "planner/logical/logical_aggregate.h"
#include "planner/logical/logical_sort.h"
#include "planner/logical/logical_join.h"
#include "common/join_type.h"

using namespace chickenDB;

namespace {
    // 若 where 是 “col = const” 形式，取出 col_id 与常量构成的索引查找键（数值或字符串）。
    // 返回是否匹配。out_key 为单分量 IndexKey，供索引点查。
    auto MatchEquality(const BoundExpression *expr, col_id_t &out_col, IndexKey &out_key) -> bool {
        if (expr == nullptr || expr->type_ != BinderExpressionType::BINARY_OP) return false;
        const auto *bin = static_cast<const BoundBinaryExpression *>(expr);
        if (bin->type_ != BinaryOpExpressionType::EQ) return false;

        const BoundExpression *l = bin->left_.get();
        const BoundExpression *r = bin->right_.get();
        const BoundColumnExpression *col = nullptr;
        const BoundConstantExpression *con = nullptr;
        if (l->type_ == BinderExpressionType::COLUMN && r->type_ == BinderExpressionType::CONSTANT) {
            col = static_cast<const BoundColumnExpression *>(l);
            con = static_cast<const BoundConstantExpression *>(r);
        } else if (l->type_ == BinderExpressionType::CONSTANT && r->type_ == BinderExpressionType::COLUMN) {
            con = static_cast<const BoundConstantExpression *>(l);
            col = static_cast<const BoundColumnExpression *>(r);
        } else {
            return false;
        }
        const auto &v = con->val_.value_;
        double out_val = 0;
        if (std::holds_alternative<int64_t>(v)) out_val = static_cast<double>(std::get<int64_t>(v));
        else if (std::holds_alternative<int>(v)) out_val = static_cast<double>(std::get<int>(v));
        else if (std::holds_alternative<double>(v)) out_val = std::get<double>(v);
        else if (std::holds_alternative<float>(v)) out_val = static_cast<double>(std::get<float>(v));
        else if (std::holds_alternative<std::string>(v)) {
            out_col = col->col_id_;
            out_key = IndexKey(std::vector<IndexKeyVal>{IndexKeyVal(std::get<std::string>(v))});
            return true;
        } else {
            return false;
        }
        out_col = col->col_id_;
        out_key = IndexKey(std::vector<double>{out_val});
        return true;
    }

    // 从 ON 条件 col = col 提取两个列引用（按 table_id 区分左右由调用方处理）。
    auto MatchColEqCol(const BoundExpression *expr, const BoundColumnExpression *&a,
                       const BoundColumnExpression *&b) -> bool {
        if (expr == nullptr || expr->type_ != BinderExpressionType::BINARY_OP) return false;
        const auto *bin = static_cast<const BoundBinaryExpression *>(expr);
        if (bin->type_ != BinaryOpExpressionType::EQ) return false;
        if (bin->left_->type_ != BinderExpressionType::COLUMN ||
            bin->right_->type_ != BinderExpressionType::COLUMN) return false;
        a = static_cast<const BoundColumnExpression *>(bin->left_.get());
        b = static_cast<const BoundColumnExpression *>(bin->right_.get());
        return true;
    }
}

auto Planner::LogicalSelectPlanner(std::unique_ptr<BoundStatement> bound_statement) -> std::unique_ptr<LogicalOperator> {
    ChickenException::AssertCondition(bound_statement->type_ == StatementType::SELECT,
                                      "[Logical] create table planner error, statement type not is select.");
    auto bound_select_statement = dynamic_cast<BoundSelectStatement*>(bound_statement.get());

    // JOIN：build 左右 scan + LogicalJoin（等值连接），再 filter -> project。
    if (bound_select_statement->has_join_) {
        auto left_scan = LogicalOperatorScan(bound_select_statement->table_id_);
        auto right_scan = LogicalOperatorScan(bound_select_statement->join_table_id_);

        auto join = std::make_unique<LogicalJoin>(JoinType::INNER);
        // 从 ON 条件提取左右 key（按 col 的 table_id 归属左/右表）。
        const BoundColumnExpression *ca = nullptr;
        const BoundColumnExpression *cb = nullptr;
        if (MatchColEqCol(bound_select_statement->join_condition_.get(), ca, cb)) {
            const table_id_t left_tid = bound_select_statement->table_id_;
            if (ca->table_id_ == left_tid) {
                join->left_keys_.push_back(ca->col_id_);
                join->right_keys_.push_back(cb->col_id_);
            } else {
                join->left_keys_.push_back(cb->col_id_);
                join->right_keys_.push_back(ca->col_id_);
            }
        }
        join->children_.push_back(std::move(left_scan));
        join->children_.push_back(std::move(right_scan));

        std::unique_ptr<LogicalOperator> root = std::move(join);
        if (bound_select_statement->where_) {
            auto filter = LogicalOperatorFilter(std::move(bound_select_statement->where_));
            filter->children_.push_back(std::move(root));
            root = std::move(filter);
        }
        auto project = LogicalOperatorProject(bound_select_statement->columns_);
        project->children_.push_back(std::move(root));
        return project;
    }

    // scan -> filter -> project
    auto scan = LogicalOperatorScan(bound_select_statement->table_id_);

    // 索引选择：WHERE 为单列等值且该列上有（单列）索引时，让 scan 走索引点查。
    // Filter 仍保留在上层做安全复核（语义恒正确）。
    if (bound_select_statement->where_) {
        col_id_t eq_col = 0;
        IndexKey eq_key;
        if (MatchEquality(bound_select_statement->where_.get(), eq_col, eq_key)) {
            auto indexes = catalog_->GetTableIndexes(bound_select_statement->table_id_);
            for (const auto *info : indexes) {
                if (info->key_cols.size() == 1 && info->key_cols[0] == eq_col) {
                    auto *logical_scan = dynamic_cast<LogicalScan *>(scan.get());
                    logical_scan->use_index_ = true;
                    logical_scan->index_name_ = info->index_name;
                    logical_scan->lookup_key_ = eq_key;
                    break;
                }
            }
        }
    }

    auto root = std::move(scan);

    if (bound_select_statement->where_) {
        auto filter = LogicalOperatorFilter(std::move(bound_select_statement->where_));
        filter->children_.push_back(std::move(root));
        root = std::move(filter);
    }

    // GROUP BY：在 project 之下插入聚合（HashAggregate）。group_ 为分组列，
    // 聚合列取自 select 列表里的聚合函数（COLUMN_AGG 标记）。
    if (!bound_select_statement->group_.empty()) {
        std::vector<col_id_t> group_cols;
        for (auto &g : bound_select_statement->group_) {
            if (g->type_ == BinderExpressionType::COLUMN) {
                group_cols.push_back(static_cast<BoundColumnExpression *>(g.get())->col_id_);
            }
        }
        // 聚合列 + 函数类型：select 列表里第一个聚合函数引用。
        col_id_t agg_col = group_cols.empty() ? 0 : group_cols[0];
        AggFuncType agg_func = AggFuncType::SUM;
        for (auto &c : bound_select_statement->columns_) {
            if (c->type_ == BinderExpressionType::COLUMN) {
                auto *col = static_cast<BoundColumnExpression *>(c.get());
                if (col->is_aggregate_) {
                    agg_col = col->col_id_;
                    const std::string &fn = col->agg_func_;
                    if (fn == "COUNT") agg_func = AggFuncType::COUNT;
                    else if (fn == "MIN") agg_func = AggFuncType::MIN;
                    else if (fn == "MAX") agg_func = AggFuncType::MAX;
                    else if (fn == "AVG") agg_func = AggFuncType::AVG;
                    else agg_func = AggFuncType::SUM;
                    break;
                }
            }
        }
        auto agg = std::make_unique<LogicalAggregate>();
        agg->group_cols_ = group_cols;
        agg->agg_col_ = agg_col;
        agg->agg_func_ = agg_func;
        agg->children_.push_back(std::move(root));
        root = std::move(agg);
        return root; // 聚合输出列：group 列 + 聚合结果列
    }

    // ORDER BY：在投影之下排序（此时各列齐全），投影保序。
    if (!bound_select_statement->order_.empty()) {
        std::vector<col_id_t> sort_cols;
        std::vector<bool> sort_desc;
        for (size_t i = 0; i < bound_select_statement->order_.size(); i++) {
            auto &o = bound_select_statement->order_[i];
            if (o->type_ == BinderExpressionType::COLUMN) {
                sort_cols.push_back(static_cast<BoundColumnExpression *>(o.get())->col_id_);
                sort_desc.push_back(i < bound_select_statement->order_desc_.size()
                                        ? bound_select_statement->order_desc_[i] : false);
            }
        }
        auto sort = std::make_unique<LogicalSort>();
        sort->col_ids_ = sort_cols;
        sort->desc_ = sort_desc;
        sort->children_.push_back(std::move(root));
        root = std::move(sort);
    }

    auto project = LogicalOperatorProject(bound_select_statement->columns_);
    project->children_.push_back(std::move(root));
    root = std::move(project);
    return root;
}

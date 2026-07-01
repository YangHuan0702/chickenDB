//
// Created by huan.yang on 2026-06-30.
//
#include "optimizer/rule/constant_folding.h"

#include <variant>

#include "binder/expression/bound_binary_expression.h"
#include "binder/expression/bound_constant_expression.h"
#include "common/enum/binary_opt_type.h"
#include "planner/logical/logical_filter.h"

using namespace chickenDB;

// ---------- 工具函数 ----------

namespace {

    // 尝试把 Value 转成 double；非数值类型返回 nullopt。
    auto ToDouble(const Value &v) -> std::optional<double> {
        if (std::holds_alternative<int>(v.value_))     return static_cast<double>(std::get<int>(v.value_));
        if (std::holds_alternative<double>(v.value_))  return std::get<double>(v.value_);
        if (std::holds_alternative<float>(v.value_))   return static_cast<double>(std::get<float>(v.value_));
        if (std::holds_alternative<int64_t>(v.value_)) return static_cast<double>(std::get<int64_t>(v.value_));
        return std::nullopt;
    }

    // 对两个数值常量做算术运算，返回结果常量；不适用时返回 nullptr。
    auto FoldArithmetic(BinaryOpExpressionType op, double lv, double rv)
        -> std::unique_ptr<BoundConstantExpression> {
        double result = 0.0;
        switch (op) {
            case BinaryOpExpressionType::ADD: result = lv + rv; break;
            case BinaryOpExpressionType::SUB: result = lv - rv; break;
            case BinaryOpExpressionType::MUL: result = lv * rv; break;
            case BinaryOpExpressionType::DRI:
                if (rv == 0.0) return nullptr; // 除零不折叠
                result = lv / rv;
                break;
            default: return nullptr;
        }
        return std::make_unique<BoundConstantExpression>(Value(result));
    }

    // 对两个数值常量做比较运算，返回 int(1)/int(0) 常量；不适用时返回 nullptr。
    auto FoldComparison(BinaryOpExpressionType op, double lv, double rv)
        -> std::unique_ptr<BoundConstantExpression> {
        bool result = false;
        switch (op) {
            case BinaryOpExpressionType::EQ:  result = (lv == rv); break;
            case BinaryOpExpressionType::NE:  result = (lv != rv); break;
            case BinaryOpExpressionType::GT:  result = (lv >  rv); break;
            case BinaryOpExpressionType::GTE: result = (lv >= rv); break;
            case BinaryOpExpressionType::LT:  result = (lv <  rv); break;
            case BinaryOpExpressionType::LTE: result = (lv <= rv); break;
            default: return nullptr;
        }
        return std::make_unique<BoundConstantExpression>(Value(result ? 1 : 0));
    }

    // 对两个字符串常量做等值/不等比较。
    auto FoldStringComparison(BinaryOpExpressionType op,
                              const std::string &ls, const std::string &rs)
        -> std::unique_ptr<BoundConstantExpression> {
        bool result = false;
        switch (op) {
            case BinaryOpExpressionType::EQ: result = (ls == rs); break;
            case BinaryOpExpressionType::NE: result = (ls != rs); break;
            default: return nullptr;
        }
        return std::make_unique<BoundConstantExpression>(Value(result ? 1 : 0));
    }

} // namespace

// ---------- ConstantFolding ----------

auto ConstantFolding::IsConstantTrue(const BoundExpression *expr) -> bool {
    if (expr == nullptr || expr->type_ != BinderExpressionType::CONSTANT) return false;
    const auto *c = dynamic_cast<const BoundConstantExpression *>(expr);
    if (c == nullptr) return false;
    const auto &v = c->val_.value_;
    if (std::holds_alternative<int>(v))     return std::get<int>(v) != 0;
    if (std::holds_alternative<int64_t>(v)) return std::get<int64_t>(v) != 0;
    if (std::holds_alternative<double>(v))  return std::get<double>(v) != 0.0;
    if (std::holds_alternative<float>(v))   return std::get<float>(v) != 0.0f;
    return false;
}

auto ConstantFolding::FoldExpr(std::unique_ptr<BoundExpression> expr)
    -> std::unique_ptr<BoundExpression> {
    if (expr == nullptr || expr->type_ != BinderExpressionType::BINARY_OP) {
        return expr;
    }

    // 取得 BoundBinaryExpression 的所有权（避免二次 dynamic_cast）。
    auto *raw = static_cast<BoundBinaryExpression *>(expr.release());
    std::unique_ptr<BoundBinaryExpression> bin(raw);

    // 先递归折叠子表达式。
    bin->left_  = FoldExpr(std::move(bin->left_));
    bin->right_ = FoldExpr(std::move(bin->right_));

    // 两侧都是常量时尝试求值。
    if (bin->left_->type_  == BinderExpressionType::CONSTANT &&
        bin->right_->type_ == BinderExpressionType::CONSTANT) {

        auto *lc = static_cast<BoundConstantExpression *>(bin->left_.get());
        auto *rc = static_cast<BoundConstantExpression *>(bin->right_.get());

        // 尝试数值折叠。
        auto lv = ToDouble(lc->val_);
        auto rv = ToDouble(rc->val_);
        if (lv.has_value() && rv.has_value()) {
            // 先尝试算术，再尝试比较。
            auto folded = FoldArithmetic(bin->type_, *lv, *rv);
            if (folded == nullptr) {
                folded = FoldComparison(bin->type_, *lv, *rv);
            }
            if (folded != nullptr) {
                return folded;
            }
        }

        // 尝试字符串比较折叠。
        const auto &lval = lc->val_.value_;
        const auto &rval = rc->val_.value_;
        if (std::holds_alternative<std::string>(lval) &&
            std::holds_alternative<std::string>(rval)) {
            auto folded = FoldStringComparison(
                bin->type_,
                std::get<std::string>(lval),
                std::get<std::string>(rval));
            if (folded != nullptr) {
                return folded;
            }
        }
    }

    return bin;
}

auto ConstantFolding::Apply(std::unique_ptr<LogicalOperator> plan)
    -> std::unique_ptr<LogicalOperator> {
    if (plan == nullptr) return nullptr;

    // 递归处理所有子节点。
    for (auto &child : plan->children_) {
        child = Apply(std::move(child));
    }

    // 对 Filter 节点折叠其谓词。
    if (plan->type_ == LogicalOperatorType::FILTER) {
        auto *filter = static_cast<LogicalFilter *>(plan.get());
        filter->condition_ = FoldExpr(std::move(filter->condition_));

        // 折叠后若谓词恒真，直接去掉 Filter 节点（返回其唯一子节点）。
        if (IsConstantTrue(filter->condition_.get())) {
            if (!plan->children_.empty()) {
                return std::move(plan->children_[0]);
            }
        }
    }

    return plan;
}

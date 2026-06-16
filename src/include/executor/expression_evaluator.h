//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

#include "executor/chunk.h"
#include "executor/like_matcher.h"
#include "binder/expression/bound_expression.h"
#include "binder/expression/bound_binary_expression.h"
#include "binder/expression/bound_column_expression.h"
#include "binder/expression/bound_constant_expression.h"
#include "binder/expression/bound_unary_expression.h"
#include "common/chicken_execption.h"
#include "common/enum/binary_opt_type.h"
#include "common/enum/unary_op_type.h"

namespace chickenDB {
    // 按 Chunk 的某一行对 BoundExpression 求值。
    //
    // 求值结果统一用 double 表示：本阶段仅支持定长数值列（NUMBER->int32,
    // DOUBLE->double），都可无损（NUMBER 范围内）表示为 double；比较/逻辑结果用
    // 1.0/0.0 表示真假。把求值逻辑放在执行层而非 binder 层，使 BoundExpression
    // 保持为纯数据，且避免 binder -> executor 的反向依赖。
    //
    // 列引用解析：BoundColumnExpression 存的是 schema 全局 col_id，而 Chunk 的列按
    // schema 顺序排列，故需要 col_id -> chunk 列下标 的映射（由算子在 Init 时构造）。
    class ExpressionEvaluator {
    public:
        using ColMap = std::unordered_map<col_id_t, size_t>;

        static auto Eval(const BoundExpression *expr, const Chunk &chunk, size_t row,
                         const ColMap &col_map) -> double {
            switch (expr->type_) {
                case BinderExpressionType::CONSTANT:
                    return ConstToDouble(static_cast<const BoundConstantExpression *>(expr)->val_);
                case BinderExpressionType::COLUMN: {
                    const auto *col = static_cast<const BoundColumnExpression *>(expr);
                    auto it = col_map.find(col->col_id_);
                    ChickenException::AssertCondition(it != col_map.end(),
                                                      "[ExpressionEvaluator] column not found in chunk");
                    return ColumnValueToDouble(chunk.GetColumn(it->second), row);
                }
                case BinderExpressionType::BINARY_OP:
                    return EvalBinary(static_cast<const BoundBinaryExpression *>(expr), chunk, row, col_map);
                case BinderExpressionType::UNARY_OP:
                    return EvalUnary(static_cast<const BoundUnaryExpression *>(expr), chunk, row, col_map);
                default:
                    throw ChickenException("[ExpressionEvaluator] unsupported expression type");
            }
        }

        // 谓词求值：非 0 即真。供 Filter / Join 谓词使用。
        static auto EvalPredicate(const BoundExpression *expr, const Chunk &chunk, size_t row,
                                  const ColMap &col_map) -> bool {
            return Eval(expr, chunk, row, col_map) != 0.0;
        }

    private:
        static auto ConstToDouble(const Value &v) -> double {
            if (std::holds_alternative<int64_t>(v.value_)) return static_cast<double>(std::get<int64_t>(v.value_));
            if (std::holds_alternative<int>(v.value_)) return static_cast<double>(std::get<int>(v.value_));
            if (std::holds_alternative<double>(v.value_)) return std::get<double>(v.value_);
            if (std::holds_alternative<float>(v.value_)) return static_cast<double>(std::get<float>(v.value_));
            if (std::holds_alternative<char>(v.value_)) return static_cast<double>(std::get<char>(v.value_));
            throw ChickenException("[ExpressionEvaluator] non-numeric constant");
        }

        static auto ColumnValueToDouble(const Vector &vec, size_t row) -> double {
            switch (vec.GetType()) {
                case ColumnType::NUMBER: return static_cast<double>(vec.GetValue<int32_t>(row));
                case ColumnType::DOUBLE: return vec.GetValue<double>(row);
                default:
                    throw ChickenException("[ExpressionEvaluator] unsupported column type for eval");
            }
        }

        // 判定一个操作数是否处于字符串域：VARCHAR/VARCHAR2 列引用，或 string 常量。
        // 仅比较类算子会据此分流到字符串比较；数值列/常量永远返回 false。
        static auto IsStringOperand(const BoundExpression *expr, const Chunk &chunk,
                                    const ColMap &col_map) -> bool {
            if (expr->type_ == BinderExpressionType::COLUMN) {
                const auto *col = static_cast<const BoundColumnExpression *>(expr);
                auto it = col_map.find(col->col_id_);
                if (it == col_map.end()) return false;
                return chunk.GetColumn(it->second).IsVar();
            }
            if (expr->type_ == BinderExpressionType::CONSTANT) {
                const auto &v = static_cast<const BoundConstantExpression *>(expr)->val_;
                return std::holds_alternative<std::string>(v.value_);
            }
            return false;
        }

        // 取字符串值。列引用 -> GetString(row)；string 常量 -> 其值。结果 string_view
        // 在 chunk / 表达式存活期内有效。
        static auto EvalString(const BoundExpression *expr, const Chunk &chunk, size_t row,
                               const ColMap &col_map) -> std::string_view {
            if (expr->type_ == BinderExpressionType::COLUMN) {
                const auto *col = static_cast<const BoundColumnExpression *>(expr);
                auto it = col_map.find(col->col_id_);
                ChickenException::AssertCondition(it != col_map.end(),
                                                  "[ExpressionEvaluator] string column not found in chunk");
                return chunk.GetColumn(it->second).GetString(row);
            }
            if (expr->type_ == BinderExpressionType::CONSTANT) {
                const auto &v = static_cast<const BoundConstantExpression *>(expr)->val_;
                ChickenException::AssertCondition(std::holds_alternative<std::string>(v.value_),
                                                  "[ExpressionEvaluator] expected string constant");
                return std::get<std::string>(v.value_);
            }
            throw ChickenException("[ExpressionEvaluator] operand is not a string value");
        }

        // 字符串比较类算子求值，返回 1.0/0.0。EQ/NE/LT/.. 走字典序；LIKE/NOT_LIKE/ILIKE
        // 走模式匹配（右操作数为 pattern）。
        static auto EvalStringComparison(const BoundBinaryExpression *e, const Chunk &chunk,
                                         size_t row, const ColMap &col_map) -> double {
            const std::string_view l = EvalString(e->left_.get(), chunk, row, col_map);
            const std::string_view r = EvalString(e->right_.get(), chunk, row, col_map);
            switch (e->type_) {
                case BinaryOpExpressionType::EQ:  return l == r ? 1.0 : 0.0;
                case BinaryOpExpressionType::NE:  return l != r ? 1.0 : 0.0;
                case BinaryOpExpressionType::GT:  return l > r ? 1.0 : 0.0;
                case BinaryOpExpressionType::GTE: return l >= r ? 1.0 : 0.0;
                case BinaryOpExpressionType::LT:  return l < r ? 1.0 : 0.0;
                case BinaryOpExpressionType::LTE: return l <= r ? 1.0 : 0.0;
                case BinaryOpExpressionType::LIKE:     return LikeMatcher::Match(l, r, false) ? 1.0 : 0.0;
                case BinaryOpExpressionType::NOT_LIKE: return LikeMatcher::Match(l, r, false) ? 0.0 : 1.0;
                case BinaryOpExpressionType::ILIKE:    return LikeMatcher::Match(l, r, true) ? 1.0 : 0.0;
                default:
                    throw ChickenException("[ExpressionEvaluator] unsupported string operator");
            }
        }

        static auto EvalBinary(const BoundBinaryExpression *e, const Chunk &chunk, size_t row,
                               const ColMap &col_map) -> double {
            // 比较/LIKE 类算子：若任一操作数处于字符串域，走字符串比较分支。
            // 数值列/常量永不命中，NUMBER/DOUBLE 路径逐字不变。
            switch (e->type_) {
                case BinaryOpExpressionType::EQ:
                case BinaryOpExpressionType::NE:
                case BinaryOpExpressionType::GT:
                case BinaryOpExpressionType::GTE:
                case BinaryOpExpressionType::LT:
                case BinaryOpExpressionType::LTE:
                case BinaryOpExpressionType::LIKE:
                case BinaryOpExpressionType::NOT_LIKE:
                case BinaryOpExpressionType::ILIKE:
                    if (IsStringOperand(e->left_.get(), chunk, col_map) ||
                        IsStringOperand(e->right_.get(), chunk, col_map)) {
                        return EvalStringComparison(e, chunk, row, col_map);
                    }
                    break;
                default:
                    break;
            }
            const double l = Eval(e->left_.get(), chunk, row, col_map);
            const double r = Eval(e->right_.get(), chunk, row, col_map);
            switch (e->type_) {
                case BinaryOpExpressionType::AND: return (l != 0.0 && r != 0.0) ? 1.0 : 0.0;
                case BinaryOpExpressionType::OR:  return (l != 0.0 || r != 0.0) ? 1.0 : 0.0;
                case BinaryOpExpressionType::GT:  return l > r ? 1.0 : 0.0;
                case BinaryOpExpressionType::GTE: return l >= r ? 1.0 : 0.0;
                case BinaryOpExpressionType::LT:  return l < r ? 1.0 : 0.0;
                case BinaryOpExpressionType::LTE: return l <= r ? 1.0 : 0.0;
                case BinaryOpExpressionType::EQ:  return l == r ? 1.0 : 0.0;
                case BinaryOpExpressionType::NE:  return l != r ? 1.0 : 0.0;
                case BinaryOpExpressionType::ADD: return l + r;
                case BinaryOpExpressionType::SUB: return l - r;
                case BinaryOpExpressionType::MUL: return l * r;
                case BinaryOpExpressionType::DRI: return l / r;
                default:
                    throw ChickenException("[ExpressionEvaluator] unsupported binary op");
            }
        }

        static auto EvalUnary(const BoundUnaryExpression *e, const Chunk &chunk, size_t row,
                              const ColMap &col_map) -> double {
            // IS_NULL / NON_NULL：基于列的 validity 位。仅当操作数是列引用时有意义。
            if (e->left_ && e->left_->type_ == BinderExpressionType::COLUMN) {
                const auto *col = static_cast<const BoundColumnExpression *>(e->left_.get());
                auto it = col_map.find(col->col_id_);
                ChickenException::AssertCondition(it != col_map.end(),
                                                  "[ExpressionEvaluator] column not found in chunk");
                bool is_valid = chunk.GetColumn(it->second).IsValid(row);
                if (e->type_ == UnaryOpType::IS_NULL) return is_valid ? 0.0 : 1.0;
                return is_valid ? 1.0 : 0.0; // NON_NULL
            }
            throw ChickenException("[ExpressionEvaluator] unary op operand must be a column");
        }
    };
}

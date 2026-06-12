//
// Created by huan.yang on 2026-06-11.
//
// 算子单测：用手工构造的算子树（不经 SQL）验证各物理算子的执行逻辑。
// 通过一个 MockSource 叶子算子喂入已知列式数据，断言上层算子输出。
//
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "executor/chunk.h"
#include "planner/physical/physical_operator.h"
#include "planner/physical/physical_filter.h"
#include "planner/physical/physical_project.h"
#include "planner/physical/physical_limit.h"
#include "planner/physical/physical_distinct.h"
#include "planner/physical/agg/physical_hash_aggregate.h"
#include "planner/physical/sort/physical_in_memory_sort.h"
#include "planner/physical/join/physical_nested_loop_join.h"
#include "planner/physical/join/physical_hash_join.h"
#include "binder/expression/bound_binary_expression.h"
#include "binder/expression/bound_column_expression.h"
#include "binder/expression/bound_constant_expression.h"
#include "common/value.h"

using namespace chickenDB;

namespace {
    // 一个只吐一个预置 chunk 的叶子算子，供单测构造数据。
    class MockSource : public PhysicalOperator {
    public:
        MockSource(std::vector<ColumnType> types, std::vector<col_id_t> ids,
                   std::vector<std::vector<double>> rows)
            : PhysicalOperator(PhysicalOperatorType::SeqScan),
              types_(std::move(types)), ids_(std::move(ids)), rows_(std::move(rows)) {}

        auto Init() -> void override { emitted_ = false; }
        auto Close() -> void override {}
        auto Next() -> Chunk * override {
            if (emitted_) return nullptr;
            emitted_ = true;
            chunk_.Init(types_, rows_.empty() ? 1 : rows_.size());
            chunk_.SetColIds(ids_);
            for (size_t r = 0; r < rows_.size(); r++) {
                for (size_t c = 0; c < types_.size(); c++) {
                    if (types_[c] == ColumnType::NUMBER) {
                        chunk_.GetColumn(c).SetValue<int32_t>(r, static_cast<int32_t>(rows_[r][c]));
                    } else {
                        chunk_.GetColumn(c).SetValue<double>(r, rows_[r][c]);
                    }
                }
            }
            chunk_.SetCount(rows_.size());
            return &chunk_;
        }

    private:
        std::vector<ColumnType> types_;
        std::vector<col_id_t> ids_;
        std::vector<std::vector<double>> rows_;
        Chunk chunk_;
        bool emitted_{false};
    };

    auto MakeSource(std::vector<std::vector<double>> rows) -> std::unique_ptr<MockSource> {
        // 两列：col_id 1 (NUMBER), col_id 2 (NUMBER)
        return std::make_unique<MockSource>(
            std::vector<ColumnType>{ColumnType::NUMBER, ColumnType::NUMBER},
            std::vector<col_id_t>{1, 2}, std::move(rows));
    }

    // 收集算子全部输出行（每行各列 double）。
    auto Drain(PhysicalOperator *op) -> std::vector<std::vector<double>> {
        std::vector<std::vector<double>> out;
        op->Init();
        while (Chunk *c = op->Next()) {
            for (size_t r = 0; r < c->Count(); r++) {
                std::vector<double> row;
                for (size_t col = 0; col < c->ColumnCount(); col++) {
                    const Vector &v = c->GetColumn(col);
                    row.push_back(v.GetType() == ColumnType::NUMBER
                                      ? static_cast<double>(v.GetValue<int32_t>(r))
                                      : v.GetValue<double>(r));
                }
                out.push_back(std::move(row));
            }
        }
        op->Close();
        return out;
    }
}

// Filter: a > 1  (col_id 1)
TEST(Operators, FilterGreaterThan) {
    auto src = MakeSource({{1, 10}, {2, 20}, {3, 30}});
    auto pred = std::make_unique<BoundBinaryExpression>(BinaryOpExpressionType::GT);
    pred->left_ = std::make_unique<BoundColumnExpression>(1, 1);
    pred->right_ = std::make_unique<BoundConstantExpression>(Value(static_cast<int64_t>(1)));

    PhysicalFilter filter(std::move(pred));
    filter.children_.push_back(std::move(src));

    auto rows = Drain(&filter);
    ASSERT_EQ(rows.size(), 2U);
    EXPECT_EQ(rows[0][0], 2);
    EXPECT_EQ(rows[1][0], 3);
}

// Project: 只保留 col_id 2
TEST(Operators, ProjectSingleColumn) {
    auto src = MakeSource({{1, 10}, {2, 20}});
    PhysicalProject project;
    project.cols_ = {2};
    project.children_.push_back(std::move(src));

    auto rows = Drain(&project);
    ASSERT_EQ(rows.size(), 2U);
    ASSERT_EQ(rows[0].size(), 1U);
    EXPECT_EQ(rows[0][0], 10);
    EXPECT_EQ(rows[1][0], 20);
}

// Limit: start=1, offset=2 -> 跳过第1行，取2行
TEST(Operators, LimitOffset) {
    auto src = MakeSource({{1, 10}, {2, 20}, {3, 30}, {4, 40}});
    PhysicalLimit limit(1, 2);
    limit.children_.push_back(std::move(src));

    auto rows = Drain(&limit);
    ASSERT_EQ(rows.size(), 2U);
    EXPECT_EQ(rows[0][0], 2);
    EXPECT_EQ(rows[1][0], 3);
}

// Distinct: 按全部列去重
TEST(Operators, DistinctAllColumns) {
    auto src = MakeSource({{1, 10}, {1, 10}, {2, 20}, {2, 20}});
    PhysicalDistinct distinct;
    distinct.children_.push_back(std::move(src));

    auto rows = Drain(&distinct);
    ASSERT_EQ(rows.size(), 2U);
}

// HashAggregate: GROUP BY col_id 1, SUM(col_id 2)
TEST(Operators, HashAggregateGroupSum) {
    auto src = MakeSource({{1, 10}, {1, 20}, {2, 5}});
    PhysicalHashAggregateOperator agg(std::vector<col_id_t>{1}, 2);
    agg.children_.push_back(std::move(src));

    auto rows = Drain(&agg);
    // 输出列：group(1) + 聚合结果(SUM) = 2 列；两组。
    ASSERT_EQ(rows.size(), 2U);
    for (auto &row : rows) {
        ASSERT_EQ(row.size(), 2U);
        if (row[0] == 1) { EXPECT_EQ(row[1], 30); }
        else if (row[0] == 2) { EXPECT_EQ(row[1], 5); }
        else { FAIL() << "unexpected group key " << row[0]; }
    }
}

// InMemorySort: 按 col_id 1 升序
TEST(Operators, InMemorySortAscending) {
    auto src = MakeSource({{3, 30}, {1, 10}, {2, 20}});
    PhysicalInMemorySort sort(std::vector<col_id_t>{1});
    sort.children_.push_back(std::move(src));

    auto rows = Drain(&sort);
    ASSERT_EQ(rows.size(), 3U);
    EXPECT_EQ(rows[0][0], 1);
    EXPECT_EQ(rows[1][0], 2);
    EXPECT_EQ(rows[2][0], 3);
}

// NestedLoopJoin: left.col1 == right.col1
TEST(Operators, NestedLoopJoinEqui) {
    auto left = MakeSource({{1, 100}, {2, 200}});
    auto right = MakeSource({{1, 11}, {2, 22}, {1, 13}});
    PhysicalNestedLoopJoin join(std::vector<col_id_t>{1}, std::vector<col_id_t>{1});
    join.children_.push_back(std::move(left));
    join.children_.push_back(std::move(right));

    auto rows = Drain(&join);
    // left(1)匹配right两行(1,1)，left(2)匹配right一行 -> 3 行；每行 4 列。
    ASSERT_EQ(rows.size(), 3U);
    EXPECT_EQ(rows[0].size(), 4U);
}

// HashJoin: 同上结果应一致
TEST(Operators, HashJoinEqui) {
    auto left = MakeSource({{1, 100}, {2, 200}});
    auto right = MakeSource({{1, 11}, {2, 22}, {1, 13}});
    PhysicalHashJoin join(std::vector<col_id_t>{1}, std::vector<col_id_t>{1});
    join.children_.push_back(std::move(left));
    join.children_.push_back(std::move(right));

    auto rows = Drain(&join);
    ASSERT_EQ(rows.size(), 3U);
    EXPECT_EQ(rows[0].size(), 4U);
}

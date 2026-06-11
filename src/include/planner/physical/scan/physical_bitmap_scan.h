//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <unordered_map>

#include "catalog/catalog.h"
#include "buffer/table_scan_iterator.h"
#include "planner/physical/physical_operator.h"
#include "binder/expression/bound_expression.h"

namespace chickenDB {
    // 位图扫描：本应先用索引生成命中行的 bitmap，再按 bitmap 批量回表读取（适合中等
    // 选择度、多条件 AND/OR 合并 bitmap 的场景）。项目当前无索引/bitmap 结构，故 v1
    // 退化为“全表扫描 + 谓词过滤”——结果正确，但无 bitmap 的批量定位价值。
    class PhysicalBitmapScan : public PhysicalOperator {
    public:
        explicit PhysicalBitmapScan(table_id_t table_id, std::shared_ptr<Catalog> catalog,
                                    std::unique_ptr<BoundExpression> predicate)
            : PhysicalOperator(PhysicalOperatorType::BitmapScan),
              table_id_(table_id), catalog_(std::move(catalog)), predicate_(std::move(predicate)) {}
        ~PhysicalBitmapScan() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        table_id_t table_id_;
        std::shared_ptr<Catalog> catalog_;
        std::unique_ptr<BoundExpression> predicate_;

    private:
        std::unique_ptr<TableScanIterator> it_;
        Chunk scan_chunk_;
        Chunk output_;
        std::unordered_map<col_id_t, size_t> col_map_;
    };
}

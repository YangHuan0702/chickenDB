//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <vector>

#include "catalog/catalog.h"
#include "buffer/table_scan_iterator.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    // 仅索引扫描：本应只读索引覆盖的列、完全不回表。项目当前无索引，故 v1 退化为
    // “全表扫描 + 投影出 output_cols_ 指定列”——结果正确，但无 covering index 价值。
    // 待索引就绪后改为直接从索引叶子读取覆盖列。
    class PhysicalIndexOnlyScan : public PhysicalOperator {
    public:
        explicit PhysicalIndexOnlyScan(table_id_t table_id, std::shared_ptr<Catalog> catalog,
                                       std::vector<col_id_t> output_cols)
            : PhysicalOperator(PhysicalOperatorType::IndexOnlyScan),
              table_id_(table_id), catalog_(std::move(catalog)), output_cols_(std::move(output_cols)) {}
        ~PhysicalIndexOnlyScan() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        table_id_t table_id_;
        std::shared_ptr<Catalog> catalog_;
        std::vector<col_id_t> output_cols_; // 覆盖列（空 = 全部列）

    private:
        std::unique_ptr<TableScanIterator> it_;
        Chunk scan_chunk_;
        Chunk output_;
        std::vector<size_t> src_idx_;
    };
}

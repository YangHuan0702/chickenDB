//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "catalog/catalog.h"
#include "buffer/table_scan_iterator.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    // 仅索引扫描（covering index）：只从索引读出键列，完全不回表。要求 output_cols_
    // 全部包含在索引键列里（由 planner 保证）。绑定索引名后走索引 ScanAll；否则退化全扫。
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
        std::vector<col_id_t> output_cols_; // 覆盖列（空 = 索引全部键列）

        std::string index_name_; // 绑定的覆盖索引；空则退化全表扫描

    private:
        // 索引路径状态。
        bool use_index_{false};
        bool emitted_{false};
        Chunk output_;
        // 全扫回退状态。
        std::unique_ptr<TableScanIterator> it_;
        Chunk scan_chunk_;
        std::vector<size_t> src_idx_;
    };
}

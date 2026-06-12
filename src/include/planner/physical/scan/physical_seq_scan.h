//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>

#include "catalog/catalog.h"
#include "buffer/table_scan_iterator.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    class PhysicalSeqScan : public PhysicalOperator {
    public:
        explicit PhysicalSeqScan(table_id_t table_id,std::shared_ptr<Catalog> catalog) : PhysicalOperator(PhysicalOperatorType::SeqScan),
                                                                     table_id_(table_id),catalog_(std::move(catalog)){}
        ~PhysicalSeqScan() override = default;

        auto Init() -> void override;

        auto Next() -> Chunk * override;

        auto Close() -> void override;

        table_id_t table_id_;
        std::shared_ptr<Catalog> catalog_;

    private:
        std::unique_ptr<TableScanIterator> it_;
        Chunk current_chunk_;
        Chunk visible_chunk_; // MVCC 快照过滤后的输出（仅事务路径用）
    };
}

//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>

#include "catalog/catalog.h"
#include "catalog/table_catalog_entry.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    class PhysicalSeqScan : public PhysicalOperator {
    public:
        explicit PhysicalSeqScan(std::shared_ptr<Catalog> catalog) : PhysicalOperator(PhysicalOperatorType::SeqScan),
                                                                     catalog_(std::move(catalog)){}
        ~PhysicalSeqScan() override = default;

        auto Init() -> void override;

        auto Next() -> Chunk * override;

        auto Close() -> void override;

        std::shared_ptr<Catalog> catalog_;
    };
}

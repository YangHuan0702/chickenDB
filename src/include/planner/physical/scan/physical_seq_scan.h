//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>

#include "catalog/table_catalog_entry.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {

    class PhysicalSeqScan : public PhysicalOperator {
    public:
        explicit PhysicalSeqScan(const TableCatalogEntry *table_catalog_entry) : PhysicalOperator(PhysicalOperatorType::SeqScan),table_(table_catalog_entry) {}
        ~PhysicalSeqScan() override = default;

        const TableCatalogEntry *table_;
    };

}

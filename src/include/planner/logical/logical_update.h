//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include "logical_operator.h"
#include "catalog/table_catalog_entry.h"
#include "common/value.h"

namespace chickenDB {
    class LogicalUpdate : public LogicalOperator {
    public:
        explicit LogicalUpdate(const TableCatalogEntry *table) : LogicalOperator(LogicalOperatorType::UPDATE),table_(table) {}
        ~LogicalUpdate() override = default;

        const TableCatalogEntry *table_;

        std::vector<col_id_t> col_ids_;
        std::vector<Value> values_;
    };
}

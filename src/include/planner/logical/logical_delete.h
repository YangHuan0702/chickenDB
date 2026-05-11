//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include "logical_operator.h"
#include "catalog/table_catalog_entry.h"

namespace chickenDB {
    class LogicalDelete : public LogicalOperator {
    public:
        explicit LogicalDelete(const TableCatalogEntry *table) : LogicalOperator(LogicalOperatorType::DELETE),table_(table) {
        }
        ~LogicalDelete() override = default;

        const TableCatalogEntry *table_;
    };
}

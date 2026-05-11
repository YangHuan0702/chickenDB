//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include "logical_operator.h"
#include "catalog/table_catalog_entry.h"
#include "common/value.h"

namespace chickenDB {

    class LogicalInsert : public LogicalOperator {
    public:
        explicit LogicalInsert(const TableCatalogEntry *table_catalog_entry) : LogicalOperator(LogicalOperatorType::INSERT),table_catalog_entry_(table_catalog_entry) {}
        ~LogicalInsert() override = default;


        const TableCatalogEntry *table_catalog_entry_;
        std::vector<col_id_t> col_ids_;
        std::vector<Value> values_;
    };

}

//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <vector>

#include "catalog/table_catalog_entry.h"
#include "common/value.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    // Physical INSERT. Execution is dispatched by Execution::ExecuteInsert rather
    // than the Init/Next/Close pull model (mirrors PhysicalCreateTable), so the
    // operator just carries the resolved table + column ids + row-major values.
    class PhysicalInsert : public PhysicalOperator {
    public:
        explicit PhysicalInsert(const TableCatalogEntry *table,
                                std::vector<col_id_t> col_ids,
                                std::vector<Value> values)
            : PhysicalOperator(PhysicalOperatorType::INSERT),
              table_(table), col_ids_(std::move(col_ids)), values_(std::move(values)) {
        }

        ~PhysicalInsert() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        const TableCatalogEntry *table_;
        std::vector<col_id_t> col_ids_;
        std::vector<Value> values_;
    };
}

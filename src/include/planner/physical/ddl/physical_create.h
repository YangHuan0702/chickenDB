//
// Created by huan.yang on 2026-05-22.
//

#pragma once
#include "parser/column_define.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    class PhysicalCreateTable : public PhysicalOperator {
    public:
        explicit PhysicalCreateTable(std::string tableName,
                                     std::vector<ColumnDefine> column_defines) : PhysicalOperator(
                PhysicalOperatorType::CREATE_TABLE), table_name_(std::move(tableName)),
            columns_(std::move(column_defines)) {
        }

        ~PhysicalCreateTable() override = default;

        auto Init() -> void override;

        auto Next() -> Chunk * override;

        auto Close() -> void override;

        std::string table_name_;
        std::vector<ColumnDefine> columns_;
        uint64_t create_ts_{0};
    };
}

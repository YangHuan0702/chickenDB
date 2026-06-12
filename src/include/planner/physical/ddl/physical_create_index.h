//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <string>
#include <vector>

#include "common/types.h"
#include "index/index.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    // 物理 CREATE INDEX。执行由 Execution::ExecuteCreateIndex 分派（同 CREATE/INSERT），
    // 算子只携带索引定义。Init/Next/Close 为空实现。
    class PhysicalCreateIndex : public PhysicalOperator {
    public:
        explicit PhysicalCreateIndex(std::string index_name, table_id_t table_id,
                                     std::vector<col_id_t> key_cols, IndexType type, bool unique)
            : PhysicalOperator(PhysicalOperatorType::CREATE_INDEX),
              index_name_(std::move(index_name)), table_id_(table_id),
              key_cols_(std::move(key_cols)), index_type_(type), unique_(unique) {}
        ~PhysicalCreateIndex() override = default;

        auto Init() -> void override;
        auto Next() -> Chunk * override;
        auto Close() -> void override;

        std::string index_name_;
        table_id_t table_id_;
        std::vector<col_id_t> key_cols_;
        IndexType index_type_;
        bool unique_;
    };
}

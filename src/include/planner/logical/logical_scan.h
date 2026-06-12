//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "logical_operator.h"
#include "binder/expression/bound_expression.h"
#include "common/types.h"
#include "index/index_key.h"

namespace chickenDB {
    class LogicalScan : public LogicalOperator {
    public:
        explicit LogicalScan(table_id_t table_id) : LogicalOperator(LogicalOperatorType::SCAN), table_id_(table_id) {
        }
        ~LogicalScan() override = default;

        table_id_t table_id_;

        // 索引选择（由 LogicalSelectPlanner 在 WHERE 命中可用索引时填充）。
        // use_index_ 为真时，物理规划生成 PhysicalIndexScan(点查 lookup_key_)。
        bool use_index_{false};
        std::string index_name_;
        IndexKey lookup_key_;
    };
}

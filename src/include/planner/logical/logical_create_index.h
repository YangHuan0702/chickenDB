//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <string>
#include <vector>

#include "logical_operator.h"
#include "common/types.h"
#include "index/index.h"

namespace chickenDB {
    class LogicalCreateIndex : public LogicalOperator {
    public:
        explicit LogicalCreateIndex() : LogicalOperator(LogicalOperatorType::CREATE_INDEX) {}
        ~LogicalCreateIndex() override = default;

        std::string index_name_;
        table_id_t table_id_{0};
        std::vector<col_id_t> key_cols_;
        IndexType index_type_{IndexType::BPlusTree};
        bool unique_{false};
    };
}

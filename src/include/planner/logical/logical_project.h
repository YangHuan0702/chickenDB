//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "logical_operator.h"
#include "common/types.h"

namespace chickenDB {

    class LogicalProject : public LogicalOperator {
    public:
        explicit LogicalProject() : LogicalOperator(LogicalOperatorType::PROJECT) {}
        ~LogicalProject() override = default;

        std::vector<col_id_t> col_ids_;

    };

}

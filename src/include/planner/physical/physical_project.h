//
// Created by huan.yang on 2026-05-09.
//
#pragma once
#include <vector>

#include "physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    class PhysicalProject : public PhysicalOperator {
    public:
        explicit PhysicalProject() : PhysicalOperator(PhysicalOperatorType::Project) {
        }

        ~PhysicalProject() override = default;

        std::vector<col_id_t> cols_;
    };
}

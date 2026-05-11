//
// Created by huan.yang on 2026-05-09.
//

#pragma once
#include <vector>

#include "physical_operator.h"
#include "common/types.h"

namespace chickenDB {
    class PhysicalDistinct : public PhysicalOperator {
    public:
        explicit PhysicalDistinct() : PhysicalOperator(PhysicalOperatorType::Distinct) {
        }

        ~PhysicalDistinct() override = default;

        std::vector<col_id_t> cols_;
    };
}

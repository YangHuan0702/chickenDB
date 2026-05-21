//
// Created by huan.yang on 2026-05-21.
//

#pragma once
#include <cstdint>

namespace chickenDB {
    struct AggregateState {
        int64_t count = 0;
        double sum = 0.0;

        auto Reset() -> void {
            count = 0;
            sum = 0.0;
        }

        auto Update(double val) -> void {
            sum += val;
            count++;
        }
    };
}

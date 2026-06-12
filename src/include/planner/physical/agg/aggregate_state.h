//
// Created by huan.yang on 2026-05-21.
//

#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace chickenDB {
    // 支持的聚合函数类型。
    enum class AggFuncType {
        SUM,
        COUNT,
        MIN,
        MAX,
        AVG,
    };

    // 单个分组的聚合累加状态：一次扫描同时维护 sum/count/min/max，
    // 输出时按 AggFuncType 取对应结果（AVG = sum/count）。
    struct AggregateState {
        int64_t count = 0;
        double sum = 0.0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();

        auto Reset() -> void {
            count = 0;
            sum = 0.0;
            min = std::numeric_limits<double>::max();
            max = std::numeric_limits<double>::lowest();
        }

        auto Update(double val) -> void {
            sum += val;
            count++;
            min = std::min(min, val);
            max = std::max(max, val);
        }

        auto Result(AggFuncType type) const -> double {
            switch (type) {
                case AggFuncType::SUM:   return sum;
                case AggFuncType::COUNT: return static_cast<double>(count);
                case AggFuncType::MIN:   return count == 0 ? 0.0 : min;
                case AggFuncType::MAX:   return count == 0 ? 0.0 : max;
                case AggFuncType::AVG:   return count == 0 ? 0.0 : sum / static_cast<double>(count);
            }
            return 0.0;
        }
    };
}

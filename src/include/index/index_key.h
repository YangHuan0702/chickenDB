//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <cstdint>
#include <vector>

namespace chickenDB {
    // 索引键：支持复合键（多列）。定长数值列（NUMBER->int32, DOUBLE->double）统一以
    // double 存放并按字典序比较——对 int32 与 double 的有序性都成立（int32 可无损转
    // double）。后续若支持变长键，再扩展为类型化的字节序列。
    struct IndexKey {
        std::vector<double> vals;

        IndexKey() = default;
        explicit IndexKey(std::vector<double> v) : vals(std::move(v)) {}

        auto operator<(const IndexKey &o) const -> bool {
            const size_t n = vals.size() < o.vals.size() ? vals.size() : o.vals.size();
            for (size_t i = 0; i < n; i++) {
                if (vals[i] < o.vals[i]) return true;
                if (vals[i] > o.vals[i]) return false;
            }
            return vals.size() < o.vals.size();
        }
        auto operator==(const IndexKey &o) const -> bool {
            if (vals.size() != o.vals.size()) return false;
            for (size_t i = 0; i < vals.size(); i++) {
                if (vals[i] != o.vals[i]) return false;
            }
            return true;
        }
        auto operator<=(const IndexKey &o) const -> bool { return *this < o || *this == o; }
        auto operator>(const IndexKey &o) const -> bool { return !(*this <= o); }
        auto operator>=(const IndexKey &o) const -> bool { return !(*this < o); }
        auto operator!=(const IndexKey &o) const -> bool { return !(*this == o); }
    };
}

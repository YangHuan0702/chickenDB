//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace chickenDB {
    // 索引键的单个分量：定长数值列以 num 存放（int32 可无损转 double），变长列(VARCHAR)
    // 以 str 存放，按 is_str 分流比较。复合键由多个分量按列序组成。
    struct IndexKeyVal {
        double num{0.0};
        std::string str;
        bool is_str{false};

        IndexKeyVal() = default;
        explicit IndexKeyVal(double v) : num(v), is_str(false) {}
        explicit IndexKeyVal(std::string v) : str(std::move(v)), is_str(true) {}

        auto operator<(const IndexKeyVal &o) const -> bool {
            // 同域比较；混域（理论上不应发生，列类型固定）以 is_str 排序保证全序。
            if (is_str != o.is_str) return !is_str; // 数值 < 字符串
            return is_str ? str < o.str : num < o.num;
        }
        auto operator==(const IndexKeyVal &o) const -> bool {
            if (is_str != o.is_str) return false;
            return is_str ? str == o.str : num == o.num;
        }
    };

    // 索引键：支持复合键（多列）。数值键与字符串键可混合（不同列不同类型），按分量字典序。
    struct IndexKey {
        std::vector<IndexKeyVal> vals;

        IndexKey() = default;

        // 兼容构造：纯数值键（历史调用零改）。每分量 is_str=false。
        explicit IndexKey(std::vector<double> v) {
            vals.reserve(v.size());
            for (double d : v) vals.emplace_back(d);
        }

        // 类型化构造：分量可含字符串。
        explicit IndexKey(std::vector<IndexKeyVal> v) : vals(std::move(v)) {}

        auto operator<(const IndexKey &o) const -> bool {
            const size_t n = vals.size() < o.vals.size() ? vals.size() : o.vals.size();
            for (size_t i = 0; i < n; i++) {
                if (vals[i] < o.vals[i]) return true;
                if (o.vals[i] < vals[i]) return false;
            }
            return vals.size() < o.vals.size();
        }
        auto operator==(const IndexKey &o) const -> bool {
            if (vals.size() != o.vals.size()) return false;
            for (size_t i = 0; i < vals.size(); i++) {
                if (!(vals[i] == o.vals[i])) return false;
            }
            return true;
        }
        auto operator<=(const IndexKey &o) const -> bool { return *this < o || *this == o; }
        auto operator>(const IndexKey &o) const -> bool { return !(*this <= o); }
        auto operator>=(const IndexKey &o) const -> bool { return !(*this < o); }
        auto operator!=(const IndexKey &o) const -> bool { return !(*this == o); }
    };
}

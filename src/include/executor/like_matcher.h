//
// Created by huan.yang on 2026-06-16.
//
#pragma once
#include <cctype>
#include <string_view>

namespace chickenDB {
    // SQL LIKE 模式匹配。pattern 中 '%' 匹配任意长度（含 0）任意字符，'_' 匹配单个
    // 任意字符，其余字符按字面匹配。ci=true 时大小写不敏感（ILIKE）。
    //
    // 采用经典双指针回溯：遇到 '%' 记下其位置与当前 text 位置，失配时回退到最近的
    // '%' 并让其多吞一个字符。时间复杂度对一般模式接近线性，最坏 O(n*m)。
    class LikeMatcher {
    public:
        static auto Match(std::string_view text, std::string_view pattern, bool ci = false) -> bool {
            size_t ti = 0;   // text 游标
            size_t pi = 0;   // pattern 游标
            size_t star = std::string_view::npos; // 最近一个 '%' 在 pattern 中的位置
            size_t mark = 0; // 对应该 '%' 时 text 的回溯位置

            while (ti < text.size()) {
                if (pi < pattern.size() && (pattern[pi] == '_' || CharEq(pattern[pi], text[ti], ci))) {
                    ti++;
                    pi++;
                } else if (pi < pattern.size() && pattern[pi] == '%') {
                    star = pi;
                    mark = ti;
                    pi++; // 先假设 '%' 匹配空串
                } else if (star != std::string_view::npos) {
                    pi = star + 1; // 回退：让上一个 '%' 多吞一个字符
                    mark++;
                    ti = mark;
                } else {
                    return false;
                }
            }
            // text 耗尽：pattern 剩余必须全是 '%'。
            while (pi < pattern.size() && pattern[pi] == '%') pi++;
            return pi == pattern.size();
        }

    private:
        static auto CharEq(char a, char b, bool ci) -> bool {
            if (!ci) return a == b;
            return std::tolower(static_cast<unsigned char>(a)) ==
                   std::tolower(static_cast<unsigned char>(b));
        }
    };
}

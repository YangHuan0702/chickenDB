//
// Created by huan.yang on 2026-06-16.
//
#pragma once
#include <string>

namespace chickenDB {
    // 排序物化行的单元格：定长列存 num，变长列(VARCHAR)存 str。比较时按 is_str 分流。
    // 用于 InMemorySort/TopN/ExternalSort 把 chunk 行物化为可排序的内存表示。
    struct SortCell {
        double num{0.0};
        std::string str;
        bool is_str{false};
    };
}

//
// Created by huan.yang on 2026-05-07.
//

#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

#include "common/enum/statement_type.h"
#include "common/types.h"

namespace chickenDB {
    constexpr size_t COLUMN_NAME_MAX_LEN = 32;

    struct ColDef {
        col_id_t col_id{0}; // 全局唯一
        char col_name[COLUMN_NAME_MAX_LEN]{};
        ColumnType data_type{ColumnType::NUMBER};
        uint8_t nullable{1};
        uint16_t type_param{0}; // VARCHAR(n) 的 n，或精度等
        uint32_t added_in_version{1}; // 哪个 Schema 版本加入的
        uint32_t dropped_in_version{0}; // 哪个版本删除的，0 表示还存活
        uint64_t default_value{0}; // 加列时老数据的默认值

        auto GetColumnName() const -> std::string {
            return std::string(col_name, strnlen(col_name, COLUMN_NAME_MAX_LEN));
        }

        auto SetColumnName(const std::string &name) -> void {
            std::memset(col_name, 0, COLUMN_NAME_MAX_LEN);
            std::memcpy(col_name, name.data(), std::min(name.size(), COLUMN_NAME_MAX_LEN - 1));
        }
    };
}

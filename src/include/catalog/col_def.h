//
// Created by huan.yang on 2026-05-07.
//

#pragma once
#include <cstdint>

#include "common/types.h"

namespace chickenDB {


    class ColDef {
    public:
        col_id_t col_id; // 全局唯一
        uint8_t col_name[32];
        uint8_t data_type; // INT32 / INT64 / FLOAT / VARCHAR ...
        uint8_t nullable;
        uint16_t type_param; // VARCHAR(n) 的 n，或精度等
        uint32_t added_in_version; // 哪个 Schema 版本加入的
        uint32_t dropped_in_version; // 哪个版本删除的，0 表示还存活
        uint64_t default_value; // 加列时老数据的默认值
    };

}

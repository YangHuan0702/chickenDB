//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <string>

#include "common/types.h"

namespace chickenDB {

    class StringUtil {
    public:
        static auto FormatFileName(table_id_t table_id) -> std::string;


    };

}

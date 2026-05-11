//
// Created by huan.yang on 2026-05-11.
//
#pragma once

#include "common/enum/statement_type.h"

namespace chickenDB {

    struct ColumnDefine {
        std::string name_;
        ColumnType type_;
        size_t size_;
    };

}

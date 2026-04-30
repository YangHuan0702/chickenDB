//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <cstdint>

namespace chickenDB {

    enum class StatementType : uint8_t {
        INSERT = 1,
        SELECT = 2,
        UPDATE = 3,
        DELETE = 4,

        CREATE = 5,
        DROP = 6,
    };


}

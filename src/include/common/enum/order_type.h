//
// Created by huan.yang on 2026-01-27.
//

#pragma once
#include <cstdint>

namespace chickenDB {

    enum class OrderType : uint8_t {
        INVALID = 0,
        ASCENDING = 1,
        DESCENDING = 2
    };

}

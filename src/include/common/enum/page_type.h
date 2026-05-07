//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <cstdint>

namespace chickenDB {
    enum class PageType : uint8_t {
        DATA = 1,
        INDEX = 2,
        META = 3
    };
}

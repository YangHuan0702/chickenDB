//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include <memory>

#include "lru_manager.h"

namespace chickenDB {

    class BufferManager {
    public:
        std::unique_ptr<LruManager> lru_manager_;

    };

}

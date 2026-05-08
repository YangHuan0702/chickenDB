//
// Created by huan.yang on 2026-05-07.
//
#pragma once
#include <string>
#include <unordered_map>

#include "common/constants.h"
#include "common/types.h"

namespace chickenDB {
    class Catalog {
    public:
        explicit Catalog() = default;

        ~Catalog() = default;

        std::unordered_map<std::string, obj_id_t> table_name_map_;
    };
}

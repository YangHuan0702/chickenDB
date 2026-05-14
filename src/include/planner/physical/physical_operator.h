//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include "common/operator_type.h"

namespace chickenDB {

    class PhysicalOperator {
      public:
        explicit PhysicalOperator(PhysicalOperatorType type) : type_(type) {};
        virtual ~PhysicalOperator() = default;

        virtual auto Init() -> void = 0;
        virtual auto Next() -> Chunk* = 0;
        virtual auto Close() -> void = 0;

        PhysicalOperatorType type_;
    };

}

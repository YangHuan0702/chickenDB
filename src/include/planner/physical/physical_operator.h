//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include "common/operator_type.h"
#include "executor/chunk.h"

namespace chickenDB {

    class PhysicalOperator {
      public:
        explicit PhysicalOperator(PhysicalOperatorType type) : type_(type) {};
        virtual ~PhysicalOperator() = default;

        virtual auto Init() -> void = 0;
        virtual auto Next() -> Chunk* = 0;
        virtual auto Close() -> void = 0;

        PhysicalOperatorType type_;
        std::unique_ptr<PhysicalOperator> children_;
    };

}

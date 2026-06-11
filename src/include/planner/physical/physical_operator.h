//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include <memory>
#include <vector>

#include "common/operator_type.h"
#include "executor/chunk.h"

namespace chickenDB {

    class ExecutorContext;

    class PhysicalOperator {
      public:
        explicit PhysicalOperator(PhysicalOperatorType type) : type_(type) {};
        virtual ~PhysicalOperator() = default;

        virtual auto Init() -> void = 0;
        virtual auto Next() -> Chunk* = 0;
        virtual auto Close() -> void = 0;

        // 第 i 个孩子（单孩子算子用 Child(0)）。越界返回 nullptr。
        auto Child(size_t i = 0) -> PhysicalOperator* {
            return i < children_.size() ? children_[i].get() : nullptr;
        }
        auto ChildCount() const -> size_t { return children_.size(); }

        PhysicalOperatorType type_;
        // 多孩子：Join 等需要两个输入。单孩子算子只用 children_[0]。
        std::vector<std::unique_ptr<PhysicalOperator>> children_;
        // 执行期上下文（buffer manager / catalog）。由 Execution 在执行前递归下传。
        ExecutorContext* ctx_{nullptr};
    };

}

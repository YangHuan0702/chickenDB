//
// Created by huan.yang on 2026-05-21.
//
#pragma once
#include <memory>

#include "executor_context.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    class Executor {
    public:
        explicit Executor(std::unique_ptr<ExecutorContext> context) : context_(std::move(context)) {
        }
        virtual ~Executor() = default;
        virtual auto Exec(std::unique_ptr<PhysicalOperator> plan) -> void = 0;

        std::unique_ptr<ExecutorContext> context_;
    };
}

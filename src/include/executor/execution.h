//
// Created by huan.yang on 2026-05-21.
//
#pragma once
#include "executor.h"

namespace chickenDB {

    class Execution : public Executor {
    public:
        explicit Execution(std::unique_ptr<ExecutorContext> context) : Executor(std::move(context)) {}
        virtual ~Execution() = default;

        auto Exec(std::unique_ptr<PhysicalOperator> plan) -> void override;

    private:
        auto ExecuteCreate(std::unique_ptr<PhysicalOperator> plan) -> void;

    };

}

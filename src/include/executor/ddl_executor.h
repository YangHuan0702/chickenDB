//
// Created by huan.yang on 2026-05-21.
//

#pragma once
#include "executor.h"

namespace chickenDB {
    class DDLExecutor : public Executor {
    public:
        explicit DDLExecutor(std::unique_ptr<ExecutorContext> context) : Executor(std::move(context)) {
        }
        ~DDLExecutor() override = default;

        auto Exec(std::unique_ptr<PhysicalOperator> plan) -> void override;
    };
}

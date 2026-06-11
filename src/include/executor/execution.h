//
// Created by huan.yang on 2026-05-21.
//
#pragma once
#include <vector>

#include "executor.h"
#include "common/value.h"

namespace chickenDB {

    class Execution : public Executor {
    public:
        explicit Execution(std::unique_ptr<ExecutorContext> context) : Executor(std::move(context)) {}
        ~Execution() override = default;

        auto Exec(std::unique_ptr<PhysicalOperator> plan) -> void override;

        // 上一次查询（SELECT 等以 Chunk 流为输出的计划）收集到的结果行，
        // 按列式 Chunk 逐行物化为 Value，供测试断言。
        std::vector<std::vector<Value>> result_rows_;

    private:
        auto ExecuteCreate(std::unique_ptr<PhysicalOperator> plan) -> void;
        auto ExecuteInsert(std::unique_ptr<PhysicalOperator> plan) -> void;
        auto ExecuteQuery(std::unique_ptr<PhysicalOperator> plan) -> void;

        // 把执行期上下文递归下传到算子树的每个节点。
        auto AttachContext(PhysicalOperator *op) -> void;
    };

}

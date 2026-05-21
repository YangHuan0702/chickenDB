//
// Created by huan.yang on 2026-05-21.
//
#pragma once
#include "buffer/buffer_manager.h"
#include "catalog/catalog.h"
#include "planner/physical/physical_operator.h"

namespace chickenDB {
    class ExecutorContext {
    public:
        explicit ExecutorContext(std::shared_ptr<BufferManager> buffer_manager,
                                 std::shared_ptr<Catalog> catalog) : buffer_manager_(buffer_manager),
                                                                     catalog_(catalog) {
        }
        ~ExecutorContext() = default;

        std::shared_ptr<BufferManager> buffer_manager_;
        std::shared_ptr<Catalog> catalog_;
    };
}

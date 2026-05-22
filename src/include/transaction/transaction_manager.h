//
// Created by huan.yang on 2026-05-22.
//
#pragma once
#include <atomic>

namespace chickenDB {
    class TransactionManager {
    public:
        explicit TransactionManager() = default;

        ~TransactionManager() = default;

        auto GetNextTxId() -> int64_t {
            return tx_id_.fetch_add(1);
        }

        std::atomic_int64_t tx_id_;
    };
}

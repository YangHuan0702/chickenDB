//
// Created by huan.yang on 2026-01-27.
//

#pragma once
#include <stdexcept>

namespace chickenDB {
    class ChickenException : public std::runtime_error {
    public:
        explicit ChickenException(const std::string &message) : std::runtime_error(message) {
        }

        static auto AssertCondition(bool result, const std::string &message) -> void {
            if (!result) {
                throw ChickenException(message);
            }
        }
    };
}

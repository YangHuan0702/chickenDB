//
// Created by huan.yang on 2026-05-13.
//
#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace chickenDB {

    class SelectionVector {
    public:
        explicit SelectionVector(std::unique_ptr<uint16_t[]> sel) :sel_(std::move(sel)) {}
        ~SelectionVector() = default;

        auto Init(size_t capacity) -> void {
            sel_ = std::make_unique<uint16_t[]>(capacity);
            active = false;
        }

        auto Set(const std::vector<uint16_t> &indices) -> void {
            std::memcpy(sel_.get(),indices.data(),indices.size()*sizeof(uint16_t));
            active = !indices.empty();
        }


        [[nodiscard]] auto PhysicalRow(uint16_t i) const -> uint16_t {
            return active ? sel_[i] : i;
        }


        bool active{false};
        std::unique_ptr<uint16_t[]> sel_;
    };

}

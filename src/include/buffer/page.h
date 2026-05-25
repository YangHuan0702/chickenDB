//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include "common/macro.h"
#include "common/types.h"
#include <cstdint>

namespace chickenDB {
    class Page {
    public:
        Page() {
            this->data = new char[PAGE_SIZE];
        }

        virtual ~Page() {
            delete [] data;
        }

        Page(const Page &) = delete;

        Page &operator=(const Page &) = delete;

        PageId page_id_;
        char *data{nullptr};
    };
}

//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <any>
#include <string>
#include <variant>

namespace chickenDB {
    class Value {
    public:
        explicit Value(const std::variant<int,char,long long,std::string,std::monostate> value) : value_(value) {}
        ~Value() = default;

        std::variant<int,char,long long,std::string,std::monostate> value_;
    };
}

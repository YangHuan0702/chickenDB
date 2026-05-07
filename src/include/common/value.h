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
        explicit Value(const std::variant<std::monostate,int,char,long long,std::string,float,double,int64_t>& value) : value_(value) {}
        ~Value() = default;

        std::variant<std::monostate,int,char,long long,std::string,float,double,int64_t> value_;
    };
}

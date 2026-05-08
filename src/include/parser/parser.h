//
// Created by 杨欢 on 2025/1/21.
//
#pragma once
#include <string>
#include <vector>
#include <memory>

#include "statment/sql_statement.h"

namespace chickenDB {
    class Parser {
    public:
        explicit Parser() = default;
        ~Parser() = default;

        void ParserQuery(const std::string &querySQL);

    private:
        std::vector<std::unique_ptr<SQLStatement>> statements_;
    };
}

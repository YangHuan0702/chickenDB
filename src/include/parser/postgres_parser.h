//
// Created by 杨欢 on 2025/1/21.
//

#pragma once

#include <string>
#include "parser/parser.hpp"
#include "pg_functions.hpp"

namespace chickenDB {



    /**
     * paser postgres
     */
    class PostgresParser{

    public:
        explicit PostgresParser() = default;
        ~PostgresParser();
        auto Parser(const std::string &query) -> void;

    private:

    };

}
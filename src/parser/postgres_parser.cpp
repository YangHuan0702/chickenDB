//
// Created by 杨欢 on 2025/1/21.
//


#include "parser/postgres_parser.h"

#include <iostream>

namespace haloTP {

    auto PostgresParser::Parser(const std::string &query) -> void {
        duckdb_libpgquery::pg_parser_init();
        duckdb_libpgquery::parse_result result;
        duckdb_libpgquery::pg_parser_parse(query.c_str(),&result);
        std::cout << "sql parser successed" <<  std::endl;
    }

    PostgresParser::~PostgresParser() {
        duckdb_libpgquery::pg_parser_cleanup();
    }
}

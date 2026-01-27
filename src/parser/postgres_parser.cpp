//
// Created by 杨欢 on 2025/1/21.
//


#include "parser/postgres_parser.h"
#include "glog/logging.h"
#include <iostream>

namespace chickenDB {
    auto PostgresParser::Parser(const std::string &query) -> void {
        duckdb_libpgquery::pg_parser_init();
        duckdb_libpgquery::parse_result result;
        duckdb_libpgquery::pg_parser_parse(query.c_str(), &result);

        if (!result.success) {
            duckdb_libpgquery::pg_parser_cleanup();
            LOG(ERROR) << "[Parser] parse error: " << result.error_message << " at position :" << result.error_location;
            throw std::runtime_error("[Parser] parse error");
        }


        for (auto entry = result.parse_tree->head; entry; entry = entry->next) {
            auto node = (duckdb_libpgquery::PGNode *)entry->data.ptr_value;
            switch (node->type) {
                case duckdb_libpgquery::T_PGSelectStmt: {

                }
                default:
                    std::cerr << "[Parser] parser error, understand node_type " << node->type << " at position " << std::endl;
                    break;
            }
        }

    }

    PostgresParser::~PostgresParser() {
        duckdb_libpgquery::pg_parser_cleanup();
    }
}

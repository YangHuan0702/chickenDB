//
// Created by huan.yang on 2026-01-27.
//
#include "parser/parser.h"

#include "glog/logging.h"
#include "postgres_parser.hpp"

#include "parser/transformer.h"

namespace chickenDB {

    Parser::Parser() {
    }


    void Parser::ParserQuery(const std::string& querySQL) {
        LOG(INFO) << "Parsing to SQL : "<< querySQL;
        duckdb::PostgresParser parser;
        parser.Parse(querySQL);

        if (!parser.success) {
            LOG(ERROR) << "Parsing SQL : "<< querySQL << " error." << parser.error_message << " position:" << parser.error_location;
            throw std::runtime_error("Parsing failed");
        }

        Transformer transformer;
        transformer.TransformerPostgresTree(parser.parse_tree,statements_);
    }




}

//
// Created by huan.yang on 2026-01-27.
//
#include "parser/parser.h"

#include "glog/logging.h"
#include "SQLParser.h"
#include "SQLParserResult.h"

#include "parser/transformer.h"

namespace chickenDB {

    void Parser::ParserQuery(const std::string& querySQL) {
        LOG(INFO) << "Parsing to SQL : "<< querySQL;
        hsql::SQLParserResult result;
        hsql::SQLParser::parse(querySQL, &result);

        if (!result.isValid()) {
            LOG(ERROR) << "Parsing SQL : "<< querySQL << " error." << result.errorMsg() << " position:" << result.errorLine();
            throw std::runtime_error("Parsing failed");
        }

        Transformer transformer;
        transformer.TransformerAST(result,statements_);
    }




}

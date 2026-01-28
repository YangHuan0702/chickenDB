//
// Created by huan.yang on 2026-01-27.
//

#pragma once
#include "parser/sql_statement.h"
#include "parser/query_node.h"

#include <unordered_map>

namespace chickenDB {

    class SelectStatement : public SQLStatement {
    public:
        explicit SelectStatement() : SQLStatement(StatementType::SELECT) {}

        bool Equals(const SQLStatement *sql_statement) const;

        std::unordered_map<std::string,std::unique_ptr<QueryNode>> cte_map;

        std::unique_ptr<QueryNode> node;

        std::unique_ptr<SelectStatement> Copy();
    };

}

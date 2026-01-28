//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include <vector>
#include <memory>

#include "parser/query_node.h"
#include "parser/parsed_expression.h"
#include "parser/table_ref.h"

namespace chickenDB {

    class SelectNode : public QueryNode {
    public:
        SelectNode() : QueryNode(SELECT_NODE) {}
        ~SelectNode() override = default;

        std::vector<std::unique_ptr<ParsedExpression>> select_list;

        std::unique_ptr<TableRef> from_table;

        std::unique_ptr<ParsedExpression> where_clause;

        std::vector<std::unique_ptr<ParsedExpression>> groups;

        std::vector<std::unique_ptr<ParsedExpression>> distinct_on_target;

        std::unique_ptr<ParsedExpression> having;

        std::vector<std::vector<std::unique_ptr<ParsedExpression>>> values;

        const std::vector<std::unique_ptr<ParsedExpression>>& GetSelectList() const override {
            return select_list;
        }


        bool Equals(const QueryNode *other) const override;

        std::unique_ptr<QueryNode> Copy() override;

    };

}

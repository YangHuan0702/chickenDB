//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/query_node.h"
#include "parser/table_ref.h"

namespace chickenDB {
    class SubqueryRef : public TableRef {
    public:
        SubqueryRef(std::unique_ptr<QueryNode> subQuery);

        std::unique_ptr<QueryNode> subQuery;
        std::vector<std::string> column_name_alias;

        bool Equals(const TableRef *other) override;

        std::unique_ptr<TableRef> Copy() const override;
    };
}

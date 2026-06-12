//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include <string>
#include <vector>
#include "sql_statement.h"

namespace chickenDB {
    class SelectStatement : public SQLStatement {
    public:
        SelectStatement(const std::string &table)  : SQLStatement(StatementType::SELECT), table_(table) {
        }
        ~SelectStatement() override = default;

        std::string table_;

        std::vector<std::unique_ptr<ParserExpression>> columns_;

        std::unique_ptr<ParserExpression> where_;

        std::vector<std::unique_ptr<ParserExpression>> group_;
        std::unique_ptr<ParserExpression> having_;

        std::vector<std::unique_ptr<ParserExpression>> order_;
        std::vector<bool> order_desc_; // 与 order_ 并列：true=降序

        // 两表 inner equi-join：FROM table_ JOIN join_table_ ON join_condition_。
        bool has_join_{false};
        std::string join_table_;
        std::unique_ptr<ParserExpression> join_condition_;

    };
}

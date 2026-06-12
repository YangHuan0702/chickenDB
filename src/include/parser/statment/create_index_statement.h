//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <string>
#include <vector>

#include "sql_statement.h"

namespace chickenDB {
    // CREATE INDEX idx ON tbl (col, ...) [USING HASH|BITMAP]。
    // index_type_ 默认 B+树；解析器目前只取列名，类别由默认值决定（后续可扩展 USING）。
    class CreateIndexStatement : public SQLStatement {
    public:
        explicit CreateIndexStatement(std::string index_name, std::string table_name)
            : SQLStatement(StatementType::CREATE_INDEX),
              index_name_(std::move(index_name)), table_name_(std::move(table_name)) {}
        ~CreateIndexStatement() override = default;

        std::string index_name_;
        std::string table_name_;
        std::vector<std::string> columns_; // 索引列名
        bool unique_{false};
    };
}

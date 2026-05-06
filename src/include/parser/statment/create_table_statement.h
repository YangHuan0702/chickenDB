//
// Created by huan.yang on 2026-04-30.
//

#pragma once
#include <string>
#include <utility>
#include <vector>

#include "sql_statement.h"

namespace chickenDB {

    struct ColumnDefine {
        std::string name_;
        ColumnType type_;
        size_t size_;
    };


    class CreateTableStatement : public SQLStatement {
    public:
        explicit CreateTableStatement(std::string table_name) : SQLStatement(StatementType::CREATE),
                                                                       table_name_(std::move(table_name)) {
        }
        ~CreateTableStatement() override = default;

        auto AddColumn(ColumnDefine column) -> void {
            this->columns_.push_back(std::move(column));
        }

        std::vector<ColumnDefine> columns_;

        std::string table_name_;
    };
}

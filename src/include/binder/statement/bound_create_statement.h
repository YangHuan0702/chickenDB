//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include <string>
#include <vector>

#include "bound_statement.h"
#include "parser/column_define.h"

namespace chickenDB {
    class BoundCreateStatement : public BoundStatement {
    public:
        explicit BoundCreateStatement(std::string table_name) : BoundStatement(StatementType::CREATE),
                                                                table_name_(std::move(table_name)) {
        }

        ~BoundCreateStatement() override = default;

        auto AddColumn(ColumnDefine col) -> void {
            this->columns_.push_back(col);
        }

        std::string table_name_;
        std::vector<ColumnDefine> columns_;
    };
}

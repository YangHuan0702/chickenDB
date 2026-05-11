//
// Created by huan.yang on 2026-05-11.
//
#pragma once
#include "logical_operator.h"
#include "sql/SQLStatement.h"

namespace chickenDB {
    class LogicalCreateTable : public LogicalOperator {
    public:
        explicit LogicalCreateTable() : LogicalOperator(LogicalOperatorType::CREATEA_TABLE) {}
        ~LogicalCreateTable() override = default;


        std::string table_name_;
        std::vector<ColumnDefine> columns_;
    };
}

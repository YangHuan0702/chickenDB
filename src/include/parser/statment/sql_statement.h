//
// Created by huan.yang on 2026-04-30.
//
#pragma once
#include "common/enum/statement_type.h"

namespace chickenDB {
    class SQLStatement {
    public:
        SQLStatement(StatementType type) : type_(type) {}

        virtual ~SQLStatement() = default;

        StatementType type_;
    };
}

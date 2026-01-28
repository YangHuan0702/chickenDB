//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include "common/enum/statement_type.h"

namespace chickenDB {

    class SQLStatement {
    public:
        explicit SQLStatement(StatementType type) : type(type) {}
        virtual ~SQLStatement() {}

        StatementType type;
    };

}

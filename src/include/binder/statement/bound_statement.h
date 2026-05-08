//
// Created by huan.yang on 2026-05-08.
//
#pragma once
#include "common/enum/statement_type.h"

namespace chickenDB {
    class BoundStatement {
    public:
        explicit BoundStatement(StatementType type) : type_(type){}
        virtual ~BoundStatement() = default;

        StatementType type_;
    };
}

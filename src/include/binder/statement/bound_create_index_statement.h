//
// Created by huan.yang on 2026-06-11.
//
#pragma once
#include <string>
#include <vector>

#include "bound_statement.h"
#include "common/types.h"

namespace chickenDB {
    // 绑定后的 CREATE INDEX：表名/列名已解析为 table_id 与 col_id。
    class BoundCreateIndexStatement : public BoundStatement {
    public:
        explicit BoundCreateIndexStatement() : BoundStatement(StatementType::CREATE_INDEX) {}
        ~BoundCreateIndexStatement() override = default;

        std::string index_name_;
        table_id_t table_id_{0};
        std::vector<col_id_t> key_cols_;
        bool unique_{false};
    };
}

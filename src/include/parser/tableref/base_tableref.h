//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include "../../common/constants/constants.h"
#include "parser/table_ref.h"
#include <string>

namespace chickenDB {

    class BaseTableRef : public TableRef {
    public:
        BaseTableRef() : TableRef(TableReferenceType::BASE_TABLE),schema_name(DEFAULT_SCHEMA) {}

        std::string schema_name;
        std::string table_name;

    public:
        std::string ToString() const override {
            return "GET(" + schema_name + "." + table_name + ")";
        }

        bool Equals(const TableRef *other) override;

        std::unique_ptr<TableRef> Copy() const override;
    };

}

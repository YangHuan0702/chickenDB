//
// Created by huan.yang on 2026-01-27.
//
#include "parser/tableref/base_tableref.h"

namespace chickenDB {
    std::unique_ptr<TableRef> BaseTableRef::Copy() const {
        auto copy = std::make_unique<BaseTableRef>();

        copy->schema_name = schema_name;
        copy->table_name = table_name;
        copy->alias = alias;

        return std::move(copy);
    }


    bool BaseTableRef::Equals(const TableRef *other_) {
        if (!TableRef::Equals(other_)) {
            return false;
        }
        auto other = (BaseTableRef *) other_;
        return other->schema_name == schema_name && other->table_name == table_name;
    }
}

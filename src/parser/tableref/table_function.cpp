//
// Created by huan.yang on 2026-01-28.
//
#include "parser/tableref/table_function.h"

namespace chickenDB {
    std::unique_ptr<TableRef> TableFunction::Copy() const {
        auto copy = std::make_unique<TableFunction>();

        copy->function = function->Copy();
        copy->alias = alias;

        return std::move(copy);
    }

    bool TableFunction::Equals(const TableRef *other_) {
        if (!TableRef::Equals(other_)) {
            return false;
        }
        auto other = (TableFunction *) other_;
        return function->Equals(other->function.get());
    }
}

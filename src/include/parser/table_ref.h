//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include <memory>

#include "common/enum/table_ref_type.h"
#include <string>
namespace chickenDB {

    class TableRef {
    public:
        explicit TableRef(TableReferenceType type) : type(type) {}
        virtual ~TableRef() {}

        TableReferenceType type;
        std::string alias;

        virtual std::string ToString() const {
            // return string();
            return "";
        }


        void Point();

        virtual bool Equals(const TableRef *other) {
            return other && other->type == type && alias == other->alias;
        }

        virtual std::unique_ptr<TableRef> Copy() const = 0;

    };

}

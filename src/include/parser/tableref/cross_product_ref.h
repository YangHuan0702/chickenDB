//
// Created by huan.yang on 2026-01-27.
//
#pragma once
#include "parser/table_ref.h"

namespace chickenDB {
    class CrossProductRef :public TableRef{
    public:
        CrossProductRef() : TableRef(TableReferenceType::CROSS_PRODUCT) {}

        std::unique_ptr<TableRef> left;
        std::unique_ptr<TableRef> right;

        bool Equals(const TableRef *other) override;
        std::unique_ptr<TableRef> Copy() const override;

    };
}

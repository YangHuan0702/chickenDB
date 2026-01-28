//
// Created by huan.yang on 2026-01-27.
//
#include "parser/tableref/cross_product_ref.h"

namespace chickenDB {
    std::unique_ptr<TableRef> CrossProductRef::Copy() const {
        auto copy = std::make_unique<CrossProductRef>();
        copy->left = left->Copy();
        copy->right = right->Copy();
        copy->alias = alias;
        return std::move(copy);
    }

    bool CrossProductRef::Equals(const TableRef *other) {
        if (!TableRef::Equals(other)) {
            return false;
        }
        auto other_ = (CrossProductRef *)other;
        return left->Equals(other_->left.get()) && right->Equals(other_->right.get());
    }


}

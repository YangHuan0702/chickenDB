//
// Created by huan.yang on 2026-01-27.
//
#include "parser/tableref/join_ref.h"

namespace chickenDB {
    std::unique_ptr<TableRef> JoinRef::Copy() const {
        auto copy = std::make_unique<JoinRef>();
        copy->left = left->Copy();
        copy->right = right->Copy();
        copy->condition = condition->Copy();
        copy->type = type;
        copy->alias = alias;
        copy->hidden_columns = hidden_columns;
        return std::move(copy);
    }

    bool JoinRef::Equals(const TableRef *other_) {
        if (!TableRef::Equals(other_)) {
            return false;
        }
        auto other = (JoinRef *) other_;
        return left->Equals(other->left.get()) && right->Equals(other->right.get()) &&
               condition->Equals(other->condition.get()) && type == other->type;
    }
}

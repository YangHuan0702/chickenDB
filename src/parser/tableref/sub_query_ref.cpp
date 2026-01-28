//
// Created by huan.yang on 2026-01-28.
//
#include "parser/tableref/sub_query_ref.h"

namespace chickenDB {
    std::unique_ptr<TableRef> SubqueryRef::Copy() const {
        auto copy = std::make_unique<SubqueryRef>(subQuery->Copy());
        copy->alias = alias;
        copy->column_name_alias = column_name_alias;
        return std::move(copy);
    }

    bool SubqueryRef::Equals(const TableRef *other_) {
        if (!TableRef::Equals(other_)) {
            return false;
        }
        auto other = (SubqueryRef *)other_;
        return subQuery->Equals(other->subQuery.get());
    }


    SubqueryRef::SubqueryRef(std::unique_ptr<QueryNode> subQuery) : TableRef(TableReferenceType::SUBQUERY), subQuery(std::move(subQuery)) {
    }

}

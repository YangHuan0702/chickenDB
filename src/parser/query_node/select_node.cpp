//
// Created by huan.yang on 2026-01-27.
//
#include "parser/query_node/select_node.h"

namespace chickenDB {

    std::unique_ptr<QueryNode> SelectNode::Copy() {
        return nullptr;
    }

    bool SelectNode::Equals(const QueryNode *other) const {
        if (!QueryNode::Equals(other)) {
            return false;
        }

        if (this == other) {
            return true;
        }

        return true;
    }

}
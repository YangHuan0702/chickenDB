//
// Created by huan.yang on 2026-01-27.
//
#include "parser/statement/select_statement.h"

namespace chickenDB {
    std::unique_ptr<SelectStatement> SelectStatement::Copy() {
        auto ret = std::make_unique<SelectStatement>();
        for (auto &re : cte_map) {
            ret->cte_map[re.first] = re.second->Copy();
        }
        ret->node = node->Copy();
        return ret;
    }

    bool SelectStatement::Equals(const SQLStatement *other) const {
        if (type != other->type) {
            return false;
        }
        auto target = (SelectStatement *) other;
        if (cte_map.size() != target->cte_map.size()) return false;

        for (auto &entry : cte_map) {
            auto other_entry = target->cte_map.find(entry.first);
            if (other_entry == target->cte_map.end()) {
                return false;
            }
            if (!entry.second->Equals(other_entry->second.get())) {
                return false;
            }
        }
        return true;
    }

}

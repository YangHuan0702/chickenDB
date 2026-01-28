//
// Created by huan.yang on 2026-01-27.
//
#include <algorithm>

#include "parser/transformer.h"

namespace chickenDB {
    bool Transformer::TransformerOrderBy(duckdb_libpgquery::PGList *order, std::vector<OrderByNode> &result) {
        if (!order) {
            return false;
        }

        for (auto node = order->head; node; node = node->next) {
            duckdb_libpgquery::PGNode* temp = reinterpret_cast<duckdb_libpgquery::PGNode*>(node->data.ptr_value);
            if (temp->type == duckdb_libpgquery::T_PGSortBy) {
                OrderByNode order_by_node;
                auto *sort = reinterpret_cast<duckdb_libpgquery::PGSortBy *>(temp);
                duckdb_libpgquery::PGNode *target = sort->node;
                if (sort->sortby_dir == duckdb_libpgquery::PG_SORTBY_ASC || sort->sortby_dir == duckdb_libpgquery::PG_SORTBY_DEFAULT) {
                    order_by_node.type = OrderType::ASCENDING;
                } else if (sort->sortby_dir == duckdb_libpgquery::PG_SORTBY_DESC) {
                    order_by_node.type = OrderType::DESCENDING;
                } else {
                    throw std::runtime_error("unimplemented order by type");
                }
                order_by_node.expression = TransformExpression(target);
                result.push_back(OrderByNode(order_by_node.type,std::move(order_by_node.expression)));
            } else {
                throw std::runtime_error("order by list member type " + std::to_string(temp->type) + " is not supported");
            }
        }
        return true;
    }

}
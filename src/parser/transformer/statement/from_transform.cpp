//
// Created by huan.yang on 2026-01-27.
//
#include "parser/transformer.h"
#include "parser/tableref/cross_product_ref.h"

namespace chickenDB {
    std::unique_ptr<TableRef> Transformer::TransformerFrom(duckdb_libpgquery::PGList *root) {
        if (!root) {
            return nullptr;
        }

        if (root->length > 1) {
            auto result = std::make_unique<CrossProductRef>();
            CrossProductRef *cur_root = result.get();
            for (auto node = root->head; node; node = node->next) {
                duckdb_libpgquery::PGNode *n = reinterpret_cast<duckdb_libpgquery::PGNode*>(node->data.ptr_value);
                std::unique_ptr<TableRef> next = TransformerTableRefNode(n);
                if (!cur_root->left) {
                    cur_root->left = std::move(next);
                } else if (!cur_root->right) {
                    cur_root->right = std::move(next);
                } else {
                    auto old_res = std::move(result);
                    result = std::make_unique<CrossProductRef>();
                    result->left=  std::move(old_res);
                    result->right = std::move(next);
                    cur_root = result.get();
                }
            }
            return std::move(result);
        }

        duckdb_libpgquery::PGNode *n = reinterpret_cast<duckdb_libpgquery::PGNode*>(root->head->data.ptr_value);
        return TransformerTableRefNode(n);
    }

}
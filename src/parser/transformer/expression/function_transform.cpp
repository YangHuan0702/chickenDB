//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"

namespace chickenDB {

    static ExpressionType AggregateToExpressionType(std::string &fun_name) {
        if (fun_name == "count") {
            return ExpressionType::AGGREGATE_COUNT;
        } else if (fun_name == "sum") {
            return ExpressionType::AGGREGATE_SUM;
        } else if (fun_name == "min") {
            return ExpressionType::AGGREGATE_MIN;
        } else if (fun_name == "max") {
            return ExpressionType::AGGREGATE_MAX;
        } else if (fun_name == "avg") {
            return ExpressionType::AGGREGATE_AVG;
        } else if (fun_name == "first") {
            return ExpressionType::AGGREGATE_FIRST;
        } else if (fun_name == "stddev_samp") {
            return ExpressionType::AGGREGATE_STDDEV_SAMP;
        }
        return ExpressionType::INVALID;
    }

    static ExpressionType WindowToExpressionType(std::string &fun_name) {
        if (fun_name == "sum") {
            return ExpressionType::WINDOW_SUM;
        } else if (fun_name == "count") {
            return ExpressionType::WINDOW_COUNT_STAR;
        } else if (fun_name == "min") {
            return ExpressionType::WINDOW_MIN;
        } else if (fun_name == "max") {
            return ExpressionType::WINDOW_MAX;
        } else if (fun_name == "avg") {
            return ExpressionType::WINDOW_AVG;
        } else if (fun_name == "rank") {
            return ExpressionType::WINDOW_RANK;
        } else if (fun_name == "rank_dense" || fun_name == "dense_rank") {
            return ExpressionType::WINDOW_RANK_DENSE;
        } else if (fun_name == "percent_rank") {
            return ExpressionType::WINDOW_PERCENT_RANK;
        } else if (fun_name == "row_number") {
            return ExpressionType::WINDOW_ROW_NUMBER;
        } else if (fun_name == "first_value" || fun_name == "first") {
            return ExpressionType::WINDOW_FIRST_VALUE;
        } else if (fun_name == "last_value" || fun_name == "last") {
            return ExpressionType::WINDOW_LAST_VALUE;
        } else if (fun_name == "cume_dist") {
            return ExpressionType::WINDOW_CUME_DIST;
        } else if (fun_name == "lead") {
            return ExpressionType::WINDOW_LEAD;
        } else if (fun_name == "lag") {
            return ExpressionType::WINDOW_LAG;
        } else if (fun_name == "ntile") {
            return ExpressionType::WINDOW_NTILE;
        }

        return ExpressionType::INVALID;
    }

    std::unique_ptr<ParsedExpression> Transformer::TransformFuncCall(duckdb_libpgquery::PGFuncCall *root) {
        std::string data = std::string(reinterpret_cast<duckdb_libpgquery::PGValue*>(root->args->head->data.ptr_value)->val.str);
        AggregateToExpressionType(data);
        WindowToExpressionType(data);
        return nullptr;
    }


}

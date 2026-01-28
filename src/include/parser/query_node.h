//
// Created by huan.yang on 2026-01-27.
//

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "common/enum/order_type.h"
#include "parser/parsed_expression.h"

namespace chickenDB {

    enum QueryNodeType: uint8_t {SELECT_NODE = 1,SET_OPERATION_NODE = 2};

    struct OrderByNode {
        OrderByNode(){
        }
        OrderByNode(OrderType type, std::unique_ptr<ParsedExpression> expression) : type(type), expression(std::move(expression)) {}
        OrderType type;
        std::unique_ptr<ParsedExpression> expression;
    };


    class QueryNode {
    public:
        QueryNode(QueryNodeType type): type_(type) {}
        virtual ~QueryNode() {}

        QueryNodeType type_;

        bool select_distinct = false;

        std::vector<OrderByNode> orders;

        std::unique_ptr<ParsedExpression> limit;

        std::unique_ptr<ParsedExpression> offset;

        virtual const std::vector<std::unique_ptr<ParsedExpression>> & GetSelectList() const = 0;

        virtual bool Equals(const QueryNode *other) const;

        virtual std::unique_ptr<QueryNode> Copy() = 0;

    };



}

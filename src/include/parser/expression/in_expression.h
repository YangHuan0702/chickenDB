//
// Created by 杨欢 on 2026/5/6.
//

#pragma once
#include <memory>
#include <vector>

#include "expression.h"

namespace chickenDB {
    class InExpression : public ParserExpression {
    public:
        explicit InExpression() : ParserExpression(ParserExpressionType::IN) {
        }

        ~InExpression() override = default;

        std::vector<std::unique_ptr<ParserExpression> > values_;
        std::unique_ptr<ParserExpression> expr_{nullptr};
        bool is_not_in_{false};
    };
}

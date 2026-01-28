//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"
#include "parser/expression/comparison_expression.h"
#include "parser/expression/conjunction_expression.h"
#include "parser/expression/content_expression.h"
#include "common/types/value.h"
#include "parser/expression/operator_expression.h"
#include "parser/expression/case_expression.h"
#include "parser/expression/function_expression.h"

using namespace chickenDB;
using namespace std;


ExpressionType Transformer::OperatorToExpressionType(string &op) {
	if (op == "+") {
		return ExpressionType::OPERATOR_ADD;
	} else if (op == "-") {
		return ExpressionType::OPERATOR_SUBTRACT;
	} else if (op == "*") {
		return ExpressionType::OPERATOR_MULTIPLY;
	} else if (op == "/") {
		return ExpressionType::OPERATOR_DIVIDE;
	} else if (op == "||") {
		return ExpressionType::OPERATOR_CONCAT;
	} else if (op == "%") {
		return ExpressionType::OPERATOR_MOD;
	} else if (op == "=") {
		return ExpressionType::COMPARE_EQUAL;
	} else if (op == "!=" || op == "<>") {
		return ExpressionType::COMPARE_NOTEQUAL;
	} else if (op == "<") {
		return ExpressionType::COMPARE_LESSTHAN;
	} else if (op == ">") {
		return ExpressionType::COMPARE_GREATERTHAN;
	} else if (op == "<=") {
		return ExpressionType::COMPARE_LESSTHANOREQUALTO;
	} else if (op == ">=") {
		return ExpressionType::COMPARE_GREATERTHANOREQUALTO;
	} else if (op == "~~") {
		return ExpressionType::COMPARE_LIKE;
	} else if (op == "!~~") {
		return ExpressionType::COMPARE_NOTLIKE;
	} else if (op == "~") {
		return ExpressionType::COMPARE_SIMILAR;
	} else if (op == "!~") {
		return ExpressionType::COMPARE_NOTSIMILAR;
	} else if (op == "<<") {
		return ExpressionType::OPERATOR_LSHIFT;
	} else if (op == ">>") {
		return ExpressionType::OPERATOR_RSHIFT;
	} else if (op == "&") {
		return ExpressionType::OPERATOR_BITWISE_AND;
	} else if (op == "|") {
		return ExpressionType::OPERATOR_BITWISE_OR;
	} else if (op == "#") {
		return ExpressionType::OPERATOR_BITWISE_XOR;
	}
	return ExpressionType::INVALID;
}

std::unique_ptr<ParsedExpression> Transformer::TransformAExpr(duckdb_libpgquery::PGAExpr *root) {
	if (!root) {
		return nullptr;
	}
	ExpressionType target_type;
	auto name = string((reinterpret_cast<duckdb_libpgquery::PGValue *>(root->name->head->data.ptr_value))->val.str);

	switch (root->kind) {
	case duckdb_libpgquery::PG_AEXPR_DISTINCT:
		target_type = ExpressionType::COMPARE_DISTINCT_FROM;
		break;
	case duckdb_libpgquery::PG_AEXPR_IN: {
		auto left_expr = TransformExpression(root->lexpr);
		ExpressionType operator_type;
		// this looks very odd, but seems to be the way to find out its NOT IN
		if (name == "<>") {
			// NOT IN
			operator_type = ExpressionType::COMPARE_NOT_IN;
		} else {
			// IN
			operator_type = ExpressionType::COMPARE_IN;
		}
		auto result = make_unique<OperatorExpression>(operator_type, std::move(left_expr));
		TransformerExpressionList((duckdb_libpgquery::PGList *)root->rexpr, result->children);
		return std::move(result);
	} break;
	// rewrite NULLIF(a, b) into CASE WHEN a=b THEN NULL ELSE a END
	case duckdb_libpgquery::PG_AEXPR_NULLIF: {
		auto case_expr = make_unique<CaseExpression>();
		auto value = TransformExpression(root->lexpr);
		// the check (A = B)
		case_expr->check = make_unique<ComparisonExpression>(ExpressionType::COMPARE_EQUAL, value->Copy(),
		                                                     TransformExpression(root->rexpr));
		// if A = B, then constant NULL
		case_expr->result_if_true = make_unique<ContentExpression>(SQLType(SQLTypeId::SQLNULL), Value());
		// else A
		case_expr->result_if_false = std::move(value);
		return std::move(case_expr);
	} break;
	// rewrite (NOT) X BETWEEN A AND B into (NOT) AND(GREATERTHANOREQUALTO(X,
	// A), LESSTHANOREQUALTO(X, B))
	case duckdb_libpgquery::PG_AEXPR_BETWEEN:
	case duckdb_libpgquery::PG_AEXPR_NOT_BETWEEN: {
		auto between_args = reinterpret_cast<duckdb_libpgquery::PGList *>(root->rexpr);

		if (between_args->length != 2 || !between_args->head->data.ptr_value || !between_args->tail->data.ptr_value) {
			throw runtime_error("(NOT) BETWEEN needs two args");
		}

		auto between_left = TransformExpression(reinterpret_cast<duckdb_libpgquery::PGNode *>(between_args->head->data.ptr_value));
		auto between_right = TransformExpression(reinterpret_cast<duckdb_libpgquery::PGNode *>(between_args->tail->data.ptr_value));

		auto compare_left = make_unique<ComparisonExpression>(ExpressionType::COMPARE_GREATERTHANOREQUALTO,
		                                                      TransformExpression(root->lexpr), std::move(between_left));
		auto compare_right = make_unique<ComparisonExpression>(ExpressionType::COMPARE_LESSTHANOREQUALTO,
		                                                       TransformExpression(root->lexpr), std::move(between_right));
		auto compare_between = make_unique<ConjunctionExpression>(ExpressionType::CONJUNCTION_AND, std::move(compare_left),
		                                                          std::move(compare_right));
		if (root->kind == duckdb_libpgquery::PG_AEXPR_BETWEEN) {
			return std::move(compare_between);
		} else {
			return make_unique<OperatorExpression>(ExpressionType::OPERATOR_NOT, std::move(compare_between));
		}
	} break;
	// rewrite SIMILAR TO into regexp_matches('asdf', '.*sd.*')
	case duckdb_libpgquery::PG_AEXPR_SIMILAR: {
		auto left_expr = TransformExpression(root->lexpr);
		auto right_expr = TransformExpression(root->rexpr);

		vector<unique_ptr<ParsedExpression>> children;
		children.push_back(std::move(left_expr));

		auto &similar_func = reinterpret_cast<FunctionExpression &>(*right_expr);
		if (similar_func.children[1]->type != ExpressionType::VALUE_CONSTANT) {
			throw std::runtime_error("Custom escape in SIMILAR TO");
		}
		auto &constant = (ContentExpression &)*similar_func.children[1];
		if (!constant.val.is_null) {
			throw std::runtime_error("Custom escape in SIMILAR TO");
		}
		children.push_back(std::move(similar_func.children[0]));

		bool invert_similar = false;
		if (name == "!~") {
			invert_similar = true;
		}
		const auto schema = DEFAULT_SCHEMA;
		const auto regex_function = "regexp_matches";
		const auto lowercase_name = regex_function;
		auto result = make_unique<FunctionExpression>(schema, lowercase_name, children);

		if (invert_similar) {
			return std::make_unique<OperatorExpression>(ExpressionType::OPERATOR_NOT, std::move(result));
		} else {
			return std::move(result);
		}
	} break;
	default: {
		target_type = OperatorToExpressionType(name);
		if (target_type == ExpressionType::INVALID) {
			throw std::runtime_error("A_Expr transform not implemented " + std::string(name.c_str()));
		}
	}
	} // continuing default case
	auto left_expr = TransformExpression(root->lexpr);
	auto right_expr = TransformExpression(root->rexpr);
	if (!left_expr) {
		switch (target_type) {
		case ExpressionType::OPERATOR_ADD:
			return right_expr;
		case ExpressionType::OPERATOR_SUBTRACT:
			target_type = ExpressionType::OPERATOR_MULTIPLY;
			left_expr = make_unique<ContentExpression>(SQLType(SQLTypeId::TINYINT), Value::TINYINT(-1));
			break;
		default:
			throw runtime_error("Unknown unary operator");
		}
	}

	if (target_type == ExpressionType::OPERATOR_CONCAT) {
		// concat operator, create function
		vector<unique_ptr<ParsedExpression>> children;
		children.push_back(std::move(left_expr));
		children.push_back(std::move(right_expr));
		return make_unique<FunctionExpression>("concat", children);
	}

	// rewrite SIMILAR TO into regexp_matches('asdf', '.*sd.*')
	if (target_type == ExpressionType::COMPARE_SIMILAR || target_type == ExpressionType::COMPARE_NOTSIMILAR) {
		bool invert_similar = false;
		if (name == "!~") {
			// NOT SIMILAR TO
			invert_similar = true;
		}
		vector<unique_ptr<ParsedExpression>> children;
		children.push_back(std::move(left_expr));
		children.push_back(std::move(right_expr));

		const auto schema = DEFAULT_SCHEMA;
		const auto regex_function = "regexp_matches";
		const auto lowercase_name = regex_function;
		auto result = make_unique<FunctionExpression>(schema, lowercase_name, children);

		if (invert_similar) {
			return std::make_unique<OperatorExpression>(ExpressionType::OPERATOR_NOT, std::move(result));
		} else {
			return std::move(result);
		}
	}

	unique_ptr<ParsedExpression> result = nullptr;
	int type_id = static_cast<int>(target_type);
	if (type_id >= static_cast<int>(ExpressionType::BINOP_BOUNDARY_START) &&
	    type_id <= static_cast<int>(ExpressionType::BINOP_BOUNDARY_END)) {
		// binary operator
		result = make_unique<OperatorExpression>(target_type, std::move(left_expr), std::move(right_expr));
	} else if (type_id >= static_cast<int>(ExpressionType::COMPARE_BOUNDARY_START) &&
	           type_id <= static_cast<int>(ExpressionType::COMPARE_BOUNDARY_END)) {
		result = make_unique<ComparisonExpression>(target_type, std::move(left_expr), std::move(right_expr));
	} else {
		throw runtime_error("A_Expr transform not implemented.");
	}
	return result;
}

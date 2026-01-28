//
// Created by huan.yang on 2026-01-28.
//
#include "common/types/value.h"


namespace chickenDB {


Value::Value(const Value &other) : type(other.type), is_null(other.is_null), str_value(other.str_value) {
	this->value_ = other.value_;
}

Value Value::MinimumValue(TypeId type) {
	Value result;
	result.type = type;
	result.is_null = false;
	switch (type) {
	case TypeId::TINYINT:
		result.value_.tinyint = std::numeric_limits<int8_t>::min();
		break;
	case TypeId::SMALLINT:
		result.value_.smallint = std::numeric_limits<int16_t>::min();
		break;
	case TypeId::INTEGER:
		result.value_.integer = std::numeric_limits<int32_t>::min();
		break;
	case TypeId::BIGINT:
		result.value_.bigint = std::numeric_limits<int64_t>::min();
		break;
	case TypeId::FLOAT:
		result.value_.float_ = std::numeric_limits<float>::min();
		break;
	case TypeId::DOUBLE:
		result.value_.double_ = std::numeric_limits<double>::min();
		break;
	case TypeId::POINTER:
		result.value_.pointer = std::numeric_limits<uint64_t>::min();
		break;
		default:
		throw std::runtime_error( "MinimumValue requires numeric type");
	}
	return result;
}

Value Value::MaximumValue(TypeId type) {
	Value result;
	result.type = type;
	result.is_null = false;
	switch (type) {
	case TypeId::TINYINT:
		result.value_.tinyint = std::numeric_limits<int8_t>::max();
		break;
	case TypeId::SMALLINT:
		result.value_.smallint = std::numeric_limits<int16_t>::max();
		break;
	case TypeId::INTEGER:
		result.value_.integer = std::numeric_limits<int32_t>::max();
		break;
	case TypeId::BIGINT:
		result.value_.bigint = std::numeric_limits<int64_t>::max();
		break;
	case TypeId::FLOAT:
		result.value_.float_ = std::numeric_limits<float>::max();
		break;
	case TypeId::DOUBLE:
		result.value_.double_ = std::numeric_limits<double>::max();
		break;
	case TypeId::POINTER:
		result.value_.pointer = std::numeric_limits<uintptr_t>::max();
		break;
		default:
		throw std::runtime_error("MaximumValue requires numeric type");
	}
	return result;
}

Value Value::BOOLEAN(int8_t value) {
	Value result(TypeId::BOOLEAN);
	result.value_.boolean = value ? true : false;
	result.is_null = false;
	return result;
}

Value Value::TINYINT(int8_t value) {
	Value result(TypeId::TINYINT);
	result.value_.tinyint = value;
	result.is_null = false;
	return result;
}

Value Value::SMALLINT(int16_t value) {
	Value result(TypeId::SMALLINT);
	result.value_.smallint = value;
	result.is_null = false;
	return result;
}

Value Value::INTEGER(int32_t value) {
	Value result(TypeId::INTEGER);
	result.value_.integer = value;
	result.is_null = false;
	return result;
}

Value Value::BIGINT(int64_t value) {
	Value result(TypeId::BIGINT);
	result.value_.bigint = value;
	result.is_null = false;
	return result;
}

Value Value::HASH(uint64_t value) {
	Value result(TypeId::HASH);
	result.value_.hash = value;
	result.is_null = false;
	return result;
}

Value Value::POINTER(uintptr_t value) {
	Value result(TypeId::POINTER);
	result.value_.pointer = value;
	result.is_null = false;
	return result;
}

}
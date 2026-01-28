//
// Created by huan.yang on 2026-01-27.
//
#include "common/enum/statement_type.h"

namespace chickenDB {

    std::string StatementTypeToString(StatementType type) {
        std::string ans;
        switch (type) {
            case StatementType::INVALID:
                ans = "INVALID";
                break;
            case StatementType::SELECT:
                ans = "SELECT";
                break;
            case StatementType::INSERT:
                ans = "INSERT";
                break;
            case StatementType::UPDATE:
                ans = "UPDATE";
                break;
            case StatementType::DELETE:
                ans = "DELETE";
                break;
            case StatementType::DEALLOCATE:
                ans = "DEALLOCATE";
                break;
            case StatementType::PREPARE:
                ans = "PREPARE";
                break;
            case StatementType::EXECUTE:
                ans = "EXECUTE";
                break;
            case StatementType::ALTER:
                ans = "ALTER";
                break;
            case StatementType::TRANSACTION:
                ans = "TRANSACTION";
                break;
            case StatementType::COPY:
                ans = "COPY";
                break;
            case StatementType::ANALYZE:
                ans = "ANALYZE";
                break;
            case StatementType::VARIABLE_SET:
                ans = "VARIABLE_SET";
                break;
            case StatementType::CREATE_FUNC:
                ans = "CREATE_FUNC";
                break;
            case StatementType::EXPLAIN:
                ans = "EXPLAIN";
                break;
            case StatementType::DROP:
                ans = "DROP";
                break;

            case StatementType::CREATE_TABLE:
                ans = "CREATE_TABLE";
                break;
            case StatementType::CREATE_SCHEMA:
                ans = "CREATE_SCHEMA";
                break;
            case StatementType::CREATE_INDEX:
                ans = "CREATE_INDEX";
                break;
            case StatementType::CREATE_VIEW:
                ans = "CREATE_VIEW";
                break;
            case StatementType::CREATE_SEQUENCE:
                ans = "CREATE_SEQUENCE";
                break;
        }
        return ans;
    }

}

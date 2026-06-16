//
// Created by huan.yang on 2026-06-11.
//
//   输入以分号或换行结束的 SQL；多条可一行分号分隔。
//   元命令：  \q / exit / quit 退出； \h 帮助。
//
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>

#include "buffer/buffer_manager.h"
#include "catalog/catalog.h"
#include "common/chicken_execption.h"
#include "common/value.h"
#include "disk/table_manager.h"
#include "executor/session.h"
#include "transaction/log_manager.h"
#include "transaction/transaction_manager.h"
#include "transaction/version_store.h"

using namespace chickenDB;

namespace {
    constexpr const char *kReset = "\033[0m";
    constexpr const char *kBold = "\033[1m";
    constexpr const char *kDim = "\033[2m";
    constexpr const char *kCyan = "\033[36m";
    constexpr const char *kGreen = "\033[32m";
    constexpr const char *kYellow = "\033[33m";
    constexpr const char *kRed = "\033[31m";

    auto UseColor() -> bool {
        const char *no_color = std::getenv("NO_COLOR");
        if (no_color != nullptr && no_color[0] != '\0') return false;

        const char *term = std::getenv("TERM");
        if (term == nullptr) return false;
        return std::string(term) != "dumb";
    }

    auto Paint(const char *color, const std::string &text) -> std::string {
        if (!UseColor()) return text;
        return std::string(color) + text + kReset;
    }

    auto PrintWelcome(const std::string &data_dir) -> void {
        std::cout << Paint(kYellow,
            "      __\n"
            "   __( o)>\n"
            "   \\ <_. )   chickenDB\n"
            "    `---'    tiny SQL, fresh pages\n")
                  << "\n"
                  << Paint(kBold, "Welcome to chickenDB CLI") << "\n"
                  << Paint(kDim, "data dir: ") << data_dir << "\n"
                  << Paint(kDim, "tips: ") << "SQL 以分号结束；"
                  << Paint(kCyan, "\\h") << " 帮助，"
                  << Paint(kCyan, "\\q") << " 退出。\n\n";
    }

    auto Prompt(size_t statement_no) -> std::string {
        std::ostringstream os;
        os << Paint(kGreen, "chickendb") << Paint(kDim, "[") << statement_no
           << Paint(kDim, "]") << Paint(kYellow, " => ");
        return os.str();
    }

    // 把一个 Value 转成可打印字符串。
    auto ValueToString(const Value &v) -> std::string {
        const auto &var = v.value_;
        if (std::holds_alternative<std::monostate>(var)) return "NULL";
        if (std::holds_alternative<int>(var)) return std::to_string(std::get<int>(var));
        if (std::holds_alternative<int64_t>(var)) return std::to_string(std::get<int64_t>(var));
        if (std::holds_alternative<double>(var)) {
            std::ostringstream os; os << std::get<double>(var); return os.str();
        }
        if (std::holds_alternative<float>(var)) {
            std::ostringstream os; os << std::get<float>(var); return os.str();
        }
        if (std::holds_alternative<char>(var)) return std::string(1, std::get<char>(var));
        if (std::holds_alternative<std::string>(var)) return std::get<std::string>(var);
        return "?";
    }

    // 把结果行集打印成对齐的表格。
    auto PrintResult(const std::vector<std::string> &columns, const std::vector<std::vector<Value>> &rows) -> void {
        size_t ncols = columns.size();
        for (const auto &row : rows) {
            ncols = std::max(ncols, row.size());
        }
        if (ncols == 0) {
            std::cout << "(0 rows)\n";
            return;
        }

        std::vector<std::string> headers;
        headers.reserve(ncols);
        for (size_t c = 0; c < ncols; c++) {
            if (c < columns.size() && !columns[c].empty()) {
                headers.push_back(columns[c]);
            } else {
                headers.push_back("column_" + std::to_string(c + 1));
            }
        }

        std::vector<size_t> width(ncols, 0);
        for (size_t c = 0; c < ncols; c++) {
            width[c] = headers[c].size();
        }
        std::vector<std::vector<std::string>> cells;
        cells.reserve(rows.size());
        for (const auto &row : rows) {
            std::vector<std::string> line;
            line.reserve(row.size());
            for (size_t c = 0; c < row.size(); c++) {
                std::string s = ValueToString(row[c]);
                if (c < ncols) width[c] = std::max(width[c], s.size());
                line.push_back(std::move(s));
            }
            cells.push_back(std::move(line));
        }
        auto sep = [&]() {
            std::cout << "+";
            for (size_t c = 0; c < ncols; c++) {
                std::cout << std::string(width[c] + 2, '-') << "+";
            }
            std::cout << "\n";
        };
        sep();
        std::cout << "|";
        for (size_t c = 0; c < ncols; c++) {
            const std::string &s = headers[c];
            std::cout << " " << s << std::string(width[c] - s.size(), ' ') << " |";
        }
        std::cout << "\n";
        sep();
        for (const auto &line : cells) {
            std::cout << "|";
            for (size_t c = 0; c < ncols; c++) {
                const std::string &s = c < line.size() ? line[c] : "";
                std::cout << " " << s << std::string(width[c] - s.size(), ' ') << " |";
            }
            std::cout << "\n";
        }
        sep();
        std::cout << "(" << rows.size() << " row" << (rows.size() == 1 ? "" : "s") << ")\n";
    }

    // 按分号切分一行里的多条语句（朴素切分，不处理字符串内分号）。
    auto SplitStatements(const std::string &line) -> std::vector<std::string> {
        std::vector<std::string> out;
        std::string cur;
        for (char ch : line) {
            if (ch == ';') {
                out.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(ch);
            }
        }
        // 末尾无分号的残余也作为一条。
        bool only_ws = cur.find_first_not_of(" \t\r\n") == std::string::npos;
        if (!only_ws) out.push_back(cur);
        return out;
    }

    auto Trim(const std::string &s) -> std::string {
        const size_t b = s.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) return "";
        const size_t e = s.find_last_not_of(" \t\r\n");
        return s.substr(b, e - b + 1);
    }
}

auto main(int argc, char **argv) -> int {
    const std::string data_dir = argc > 1 ? argv[1] : "./data/chickendb_cli";
#ifdef _WIN32
    _putenv_s("CHICKENDB_DATA_PATH", data_dir.c_str());
#else
    setenv("CHICKENDB_DATA_PATH", data_dir.c_str(), 1);
#endif

    // 组装数据库实例（与端到端测试一致的子系统装配）。
    auto lru = std::make_shared<LRUTableManager>();
    auto buffer = std::make_shared<BufferManager>(lru);
    auto catalog = std::make_shared<Catalog>(buffer);
    auto txn_mgr = std::make_shared<TransactionManager>();
    auto vstore = std::make_shared<VersionStore>();
    auto log = std::make_shared<LogManager>();
    Session session(buffer, catalog, txn_mgr, vstore, log);

    PrintWelcome(data_dir);

    std::string line;
    size_t statement_no = 1;
    while (true) {
        std::cout << Prompt(statement_no) << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break; // EOF (Ctrl-D)
        }
        const std::string trimmed = Trim(line);
        if (trimmed.empty()) continue;
        if (trimmed == "\\q" || trimmed == "exit" || trimmed == "quit") break;
        if (trimmed == "\\h" || trimmed == "help") {
            std::cout << Paint(kBold, "可用命令\n")
                      << "  SQL: CREATE TABLE/INDEX, INSERT, SELECT, DELETE, UPDATE\n"
                      << "  查询: WHERE, GROUP BY, ORDER BY, JOIN\n"
                      << "  事务: BEGIN, COMMIT, ROLLBACK\n"
                      << "  CLI:  \\h/help 查看帮助，\\q/exit/quit 退出\n";
            continue;
        }

        for (const std::string &raw : SplitStatements(line)) {
            const std::string sql = Trim(raw);
            if (sql.empty()) continue;
            try {
                session.Execute(sql);
                const auto &rows = session.LastResult();
                const auto &columns = session.LastColumnNames();
                if (!rows.empty() || !columns.empty()) {
                    PrintResult(columns, rows);
                } else {
                    std::cout << Paint(kGreen, "OK") << "\n";
                }
                statement_no++;
            } catch (const std::exception &e) {
                std::cout << Paint(kRed, "ERROR: ") << e.what() << "\n";
            }
        }
    }

    std::cout << Paint(kDim, "bye. pages flushed, coop closed.") << "\n";
    return 0;
}

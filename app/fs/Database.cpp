

#include "Database.h"
#include <cassert>
#include <exception>
#include <iostream>
#include <regex>
#include <sstream>
namespace
{
static void regexpFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    // int len1 = sqlite3_value_bytes(argv[0]);
    const char *data1 = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
    // int len2 = sqlite3_value_bytes(argv[1]);
    const char *data2 = reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));

    std::regex regex{data1, std::regex_constants::ECMAScript};
    std::smatch match;

    std::string regexStr = data2;
    std::regex_search(regexStr, match, regex);

    sqlite3_result_int(context, match.size() != 0U);
}
} // namespace
namespace fs
{
Database::Database()
{
    if (sqlite3_open("LOG.db", &m_db) != SQLITE_OK) {
        std::cerr << sqlite3_errmsg(m_db) << '\n';
        throw std::runtime_error("Failed to create or open database.");
    } else {
        assert(sqlite3_get_autocommit(m_db) && "auto commit is not enabled");
        sqlite3_create_function(m_db, "regexp", 2, SQLITE_UTF8, NULL, &regexpFunction, NULL, NULL);
        if (!executeSqlQuery(SQL_CREATE_TABLE)) {
            throw std::runtime_error("Failed to create database table.");
        }
    }
}

Database::~Database()
{
    sqlite3_free(m_errorMsg);
    sqlite3_close(m_db);
}

void Database::insert(const Watcher::DirectoryChange &record)
{
    std::lock_guard lockGuard(m_mutex);

    if (SQLITE_OK != sqlite3_prepare_v2(m_db, SQL_INSERT_FILECHANGE, SQLITE_CALC_STRLEN, &m_insertStatement, nullptr)) {
        throw std::runtime_error("sqlite3_prepare_v2 error");
    }

    const auto path = record.path.generic_string();
    bool result = sqlite3_bind_int(m_insertStatement, 1, static_cast<int>(record.changeType)) == SQLITE_OK;
    result &= sqlite3_bind_text(m_insertStatement, 2, path.c_str(), -1, nullptr) == SQLITE_OK;
    if (!result) {
        throw std::runtime_error("sqlite3_bind error");
    }

    if (sqlite3_step(m_insertStatement) != SQLITE_DONE) {
        throw std::runtime_error("sqlite3_step error");
    }

    sqlite3_finalize(m_insertStatement);
}

void Database::select(const std::string &pathRegex, const std::string &dateFrom, const std::string &dateTo) //! TODO: limit select/timeout
{
    std::lock_guard lockGuard(m_mutex);
    std::string sql{SQL_SELECT_FILECHANGES};
    std::stringstream whereCondition;
    if (!pathRegex.empty())
        whereCondition << " Path REGEXP '" << pathRegex << '\'';
    if (!dateFrom.empty()) {
        if (whereCondition.rdbuf()->in_avail()) {
            whereCondition << " AND ";
        }
        whereCondition << " Time >= '" << dateFrom << '\'';
    }
    if (!dateTo.empty()) {
        if (whereCondition.rdbuf()->in_avail()) {
            whereCondition << " AND ";
        }
        whereCondition << " Time <= '" << dateTo << '\'';
    }
    if (whereCondition.rdbuf()->in_avail()) {
        sql = sql.erase(sql.size() - 1) + " WHERE " + whereCondition.str() + " ;";
    }
    const char *psql = sql.c_str();

    if (SQLITE_OK != sqlite3_prepare_v2(m_db, psql, SQLITE_CALC_STRLEN, &m_selectStatement, nullptr)) {
        throw std::runtime_error("sqlite3_prepare_v2 error");
    }

    std::cout << std::left << std::setw(25) << sqlite3_column_name(m_selectStatement, 0)
              << std::left << std::setw(15) << sqlite3_column_name(m_selectStatement, 1)
              << sqlite3_column_name(m_selectStatement, 2) << '\n';

    while (sqlite3_step(m_selectStatement) == SQLITE_ROW) {
        const auto *datetime = sqlite3_column_text(m_selectStatement, 0);
        const auto action = std::to_string(static_cast<Watcher::ChangeType>(sqlite3_column_int(m_selectStatement, 1)));
        const auto *path = sqlite3_column_text(m_selectStatement, 2);

        std::cout << std::left << std::setw(25) << datetime
                  << std::left << std::setw(15) << action
                  << path << '\n';
    }
    std::cout << std::endl;
}

bool Database::executeSqlQuery(const std::string &sql) noexcept
{
    const bool result = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &m_errorMsg) == SQLITE_OK;

    if (m_errorMsg) {
        std::cerr << m_errorMsg << '\n';
        sqlite3_free(m_errorMsg);
    }
    return result;
}

} // namespace fs



#include "Database.h"
#include <cassert>
#include <exception>
#include <iostream>
#include <regex>
namespace
{
static void regexpFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    // int len1 = sqlite3_value_bytes(argv[0]);
    const char *data1 = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
    std::regex r{data1, std::regex_constants::ECMAScript};

    int len2 = sqlite3_value_bytes(argv[1]);
    const char *data2 = reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));
    std::smatch m;

    std::string str = data2;
    std::regex_search(str, m, r);

    // std::cout << (const char *)data1 << '\n';
    // std::cout << (const char *)data2 << std::endl;
    // if (!data1 || !data2)
    //     return;

    // // do not use fromRawData for pattern string because it may be cached internally by the regexp engine
    // QString string1 = QString::fromRawData(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar));
    // QString string2 = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar));

    // QzRegExp pattern(string1, Qt::CaseInsensitive);
    // int pos = pattern.indexIn(string2);

    sqlite3_result_int(context, m.size() != 0U); // (pos > -1) ? 1 : 0);
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
        // con.create_function('regexp', 2, regexp)
        if (!executeSqlQuery(SQL_CREATE_TABLE)) {
            throw std::runtime_error("Failed to create database table.");
        }
        select();
    }
}

Database::~Database()
{
    sqlite3_free(m_errorMsg);
    sqlite3_finalize(m_insertStatement);
    sqlite3_finalize(m_selectStatement);
    sqlite3_close(m_db);
}

void Database::insert(const Watcher::DirectoryChange &record)
{
    std::lock_guard lockGuard(m_mutex);

    if (SQLITE_OK != sqlite3_prepare_v2(m_db, SQL_INSERT_FILECHANGE, SQLITE_CALC_STRLEN, &m_insertStatement, nullptr)) {
        throw std::runtime_error("sqlite3_prepare_v2 error");
    }

    const auto path = record.path.generic_string();
    bool result = sqlite3_bind_int(m_insertStatement, 1, static_cast<int>(record.event)) == SQLITE_OK;
    result &= sqlite3_bind_text(m_insertStatement, 2, path.c_str(), -1, nullptr) == SQLITE_OK;
    if (!result) {
        throw std::runtime_error("sqlite3_bind error");
    }

    if (sqlite3_step(m_insertStatement) != SQLITE_DONE) {
        throw std::runtime_error("sqlite3_step error");
    }

    sqlite3_finalize(m_insertStatement);
}

void Database::select() //! TODO: limit select/timeout
{
    std::lock_guard lockGuard(m_mutex);

    if (SQLITE_OK != sqlite3_prepare_v2(m_db, SQL_SELECT_FILECHANGES, SQLITE_CALC_STRLEN, &m_selectStatement, nullptr)) {
        throw std::runtime_error("sqlite3_prepare_v2 error");
    }

    std::cout << std::left << std::setw(25) << sqlite3_column_name(m_selectStatement, 0)
              << std::left << std::setw(15) << sqlite3_column_name(m_selectStatement, 1)
              << sqlite3_column_name(m_selectStatement, 2) << '\n';

    while (sqlite3_step(m_selectStatement) == SQLITE_ROW) {
        const auto *datetime = sqlite3_column_text(m_selectStatement, 0);
        const auto action = std::to_string(static_cast<Watcher::DirectoryChangeEvent>(sqlite3_column_int(m_selectStatement, 1)));
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


// hmm, use wrapper or pure SQLite?  https://github.com/SRombauts/SQLiteCpp
// well since it's only 1 table ... and use of any 3rd party is not advised ... let's try pure
#pragma once
#include "Watcher.h"
#include "sqlite3.h"
#include <exception>
#include <iostream>

namespace fs
{
class Database final
{
  public:
    Database()
    {
        if (sqlite3_open("LOG.db", &m_db) != SQLITE_OK) {
            std::cerr << sqlite3_errmsg(m_db) << '\n';
            throw std::runtime_error("Failed to create or open database.");
        } else {
            if (!executeSqlQuery(SQL_CREATE_TABLE)) {
                throw std::runtime_error("Failed to create database table.");
            }
        }
    }
    ~Database()
    {
        sqlite3_free(m_errMsg);
        sqlite3_close(m_db);
    }

    void insert(const Watcher::DirectoryChange &record)
    {
    }
    void select()
    {
    }

  private:
    static constexpr auto SQL_CREATE_TABLE = "CREATE TABLE IF NOT EXISTS FileChanges("
                                             "Time TEXT NOT NULL,"
                                             "Action INT NOT NULL,"
                                             "isExecuted BOOLEAN DEFAULT(FALSE),"
                                             "Path TEXT NOT NULL);";

    bool executeSqlQuery(const std::string &sql) noexcept
    {
        const bool result = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &m_errMsg) == SQLITE_OK;

        if (m_errMsg) {
            std::cerr << m_errMsg << '\n';
            sqlite3_free(m_errMsg);
        }
        return result;
    }

    sqlite3 *m_db;
    char *m_errMsg;
};
} // namespace fs

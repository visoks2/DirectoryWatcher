
// hmm, use wrapper or pure SQLite?  https://github.com/SRombauts/SQLiteCpp
// well since it's only 1 table ... and use of any 3rd party is not advised ... let's try pure
// UPDATE: i regret this ^

#pragma once
#include "Watcher.h"
#include "sqlite3.h"
#include <mutex>

namespace fs
{

// at this point i'm not thinking about future improvements of Database class, so will go with fast/rough implementation
// also very likely error prone if someone corrupts/updates/messes up db
class Database final
{
  public:
    Database();
    ~Database();

    void insert(const Watcher::DirectoryChange &record);
    void select();

  private:
    void prepare();
    bool executeSqlQuery(const std::string &sql) noexcept;
    static constexpr int SQLITE_CALC_STRLEN = -1;
    static constexpr auto SQL_INSERT_FILECHANGE = "INSERT INTO FileChanges(Time,Action,Path) VALUES(datetime('now'), ?, ?);";
    static constexpr auto SQL_SELECT_FILECHANGES = "SELECT Time,Action,Path FROM FileChanges WHERE Path REGEXP '(([A-Z])\\w+)';";
    static constexpr auto SQL_CREATE_TABLE = "CREATE TABLE IF NOT EXISTS FileChanges("
                                             "Time DATETIME NOT NULL,"
                                             "Action INT NOT NULL,"
                                             "Path TEXT NOT NULL);";
    sqlite3_stmt *m_insertStatement;
    sqlite3_stmt *m_selectStatement;

    sqlite3 *m_db;
    char *m_errorMsg;
    std::mutex m_mutex;
};
} // namespace fs

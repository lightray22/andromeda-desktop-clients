#ifndef LIBA2_SQLITEDATABASE_H_
#define LIBA2_SQLITEDATABASE_H_

#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <string>

#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/Utilities.hpp"
#include "andromeda/database/MixedInput.hpp"
#include "andromeda/database/MixedOutput.hpp"

struct sqlite3;
struct sqlite3_stmt;

namespace Andromeda {
namespace Database {

/** Provides a simple C++ interface with exceptions over sqlite3 - THREAD SAFE (INTERNAL LOCK) */
class SqliteDatabase
{
public:

    /** Base Exception for sqlite issues */
    class Exception : public BaseException { public:
        /** @param message error message */
        explicit Exception(const std::string& message) :
            BaseException("Sqlite Error: "+message) {}; };

    /** 
     * Opens an SQLite database with the given path
     * @param path path to database file, will be created if not existing
     * @throws Exception if it fails 
     */
    explicit SqliteDatabase(const std::string& path);

    virtual ~SqliteDatabase();
    DELETE_COPY(SqliteDatabase)
    DELETE_MOVE(SqliteDatabase)

    using Row = std::map<std::string,MixedOutput>;
    using RowList = std::list<Row>;

    /**
     * Perform a query against the database - THREAD SAFE (INTERNAL LOCK)
     * @param sql the SQL query string
     * @param params array of parameters to substitute
     * @param[out] rows reference to list of rows to output
     * @return size_t number of rows matched (valid for INSERT, UPDATE, DELETE only)
     * @throws Exception if the query fails
     */
    size_t query(const std::string& sql, const MixedParams& params, RowList& rows);

private:

    /** Print and return an error text string if rc != SQLITE_OK */
    std::string print_rc(int rc) const;

    using VoidFunc = std::function<void()>;
    /**
     * Handle the given rc if != SQLITE_OK
     * @param rc sqlite return code/result
     * @param func optional function to run before throwing
     * @throws Exception if the rc is not SQLITE_OK
     */
    void check_rc(int rc, const VoidFunc& func = nullptr) const;

    mutable Debug mDebug;
    std::mutex mMutex;

    // sqlite database instance
    sqlite3* mDatabase { nullptr };
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_SQLITEDATABASE_H_

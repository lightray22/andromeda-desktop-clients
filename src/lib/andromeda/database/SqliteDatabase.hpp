#ifndef LIBA2_SQLITEDATABASE_H_
#define LIBA2_SQLITEDATABASE_H_

#include <functional>
#include <list>
#include <mutex>
#include <string>

#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/Utilities.hpp"
#include "andromeda/database/MixedValue.hpp"

struct sqlite3;
struct sqlite3_stmt;

namespace Andromeda {
namespace Database {

/** 
 * Provides a simple C++ interface with exceptions over sqlite3
 * THREAD SAFE (INTERNAL LOCKS)
 */
class SqliteDatabase
{
public:

    /** Base Exception for sqlite issues */
    class Exception : public BaseException { public:
        /** @param message error message */
        explicit Exception(const std::string& message) :
            BaseException("Sqlite Error: "+message) {}; };

    /** Exception indicating already in a transaction */
    class AlreadyTransactionException : public Exception { public:
        AlreadyTransactionException() : Exception("already in transaction") {}; };

    /** 
     * Opens an SQLite database with the given path
     * @param path path to database file, will be created if not existing
     * @throws Exception if it fails 
     */
    explicit SqliteDatabase(const std::string& path);

    virtual ~SqliteDatabase();
    DELETE_COPY(SqliteDatabase)
    DELETE_MOVE(SqliteDatabase)

    using UniqueLock = std::lock_guard<std::mutex>;

    using Row = MixedParams;
    using RowList = std::list<Row>;

    /**
     * Sends an SQL query down to the database, possibly beginning a transaction
     * @param sql the SQL query string, with placeholder data values
     * @param params param replacements for the prepared statement
     * @param[out] rows reference to list of rows to output
     * @return size_t number of rows matched (valid for INSERT, UPDATE, DELETE only)
     * @throws Exception if the query fails
     */
    virtual size_t query(const std::string& sql, const MixedParams& params, RowList& rows);

    /** 
     * Same as query() but assumes no rows output
     * @throws Exception if the query fails or rows output is not empty
     */
    size_t query(const std::string& sql, const MixedParams& params);

    // pre-locked/no BEGIN TRANSACTION versions of the above
    size_t query(const std::string& sql, const MixedParams& params, const UniqueLock& lock);
    size_t query(const std::string& sql, const MixedParams& params, RowList& rows, const UniqueLock& lock);

    using LockedFunc = std::function<void(const UniqueLock& lock)>;
    /**
     * Runs the given function as a transaction, with auto commit/rollback at the end
     * @param func function to run under a atomic locked transaction (must call pre-locked query!)
     * @throws AlreadyTransactionException if already in a transaction
     * @throws Exception if any queries fail (will auto-rollback)
     */
    void transaction(const LockedFunc& func);

    /** 
     * Rolls back the current database transaction 
     * @throws Exception if the query fails
     */
    void rollback();

    /** 
     * Commits the current database transaction 
     * @throws Exception if the query fails
     */
    void commit();

protected:

    SqliteDatabase() : mDebug(__func__,this) { } // unit test

private:

    using VoidFunc = std::function<void()>;
    /**
     * Handle the given rc if != SQLITE_OK
     * @param rc sqlite return code/result
     * @param func optional function to run before throwing
     * @throws Exception if the rc is not SQLITE_OK
     */
    void check_rc(int rc, const VoidFunc& func = nullptr) const;

    /** Print and return an error text string if rc != SQLITE_OK */
    std::string print_rc(int rc) const;

    mutable Debug mDebug;
    std::mutex mMutex;

    // sqlite database instance
    sqlite3* mDatabase { nullptr };
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_SQLITEDATABASE_H_

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
#include "andromeda/database/MixedOutput.hpp"

struct sqlite3;
struct sqlite3_stmt;

namespace Andromeda {
namespace Database {

/** Provides a simple C++ interface with exceptions over sqlite3 */ 
// TODO !! better comment... mention thread safe
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

    size_t query(const std::string& sql, RowList& rows);
    // TODO !! comment retval is only valid for INSERT, UPDATE, DELETE

private:

    // TODO !! comments
    std::string print_rc(int rc) const;
    using VoidFunc = std::function<void()>;
    void check_rc(int rc, const VoidFunc& func = nullptr) const;

    mutable Debug mDebug;
    std::mutex mMutex;

    // sqlite database instance
    sqlite3* mDatabase { nullptr };
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_SQLITEDATABASE_H_

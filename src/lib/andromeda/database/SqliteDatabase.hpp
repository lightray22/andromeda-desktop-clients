#ifndef LIBA2_SQLITEDATABASE_H_
#define LIBA2_SQLITEDATABASE_H_

#include <string>

#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/Utilities.hpp"

struct sqlite3;

namespace Andromeda {
namespace Database {

/** Provides a nice C++ interface over sqlite3 */
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

private:

    Debug mDebug;

    // sqlite database instance
    sqlite3* mDatabase { nullptr };
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_SQLITEDATABASE_H_

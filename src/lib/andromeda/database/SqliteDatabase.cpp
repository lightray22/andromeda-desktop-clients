
#include <filesystem>

#include <sqlite3.h>

#include "SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {

namespace { // anonymous

Debug sDebug("libsqlite3",nullptr); // NOLINT(cert-err58-cpp)

void sqlite_error(void* pArg, int errCode, const char* zMsg){
    SDBG_ERROR("... sqlite:" << errCode << "(" << zMsg << ")");
}

} // anonymous namespace

/*****************************************************/
SqliteDatabase::SqliteDatabase(const std::string& path) :
    mDebug(__func__,this)
{
    // the sqlite3 error log is static/global
    sqlite3_config(SQLITE_CONFIG_LOG, sqlite_error, nullptr);
    
    MDBG_INFO("(path:" << path << ")");

    const int rc = sqlite3_open(path.c_str(), &mDatabase);
    if (rc != SQLITE_OK)
    {
        const std::string errmsg(sqlite3_errmsg(mDatabase));
        MDBG_ERROR("... open returned:" << rc << " (" << errmsg << ")");

        sqlite3_close(mDatabase);
        throw Exception(errmsg);
    }
}

/*****************************************************/
SqliteDatabase::~SqliteDatabase()
{
    MDBG_INFO("()");

    sqlite3_close(mDatabase);
}

} // namespace Database
} // namespace Andromeda

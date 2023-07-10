
#include <filesystem>
#include <sstream>
#include <utility>

#include <sqlite3.h>

#include "SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {

namespace { // anonymous

Debug sDebug("libsqlite3",nullptr); // NOLINT(cert-err58-cpp)

void print_error(void* pArg, int errCode, const char* zMsg){
    SDBG_ERROR("... code:" << errCode << " (" << zMsg << ")");
}

struct SqliteLog { SqliteLog() noexcept { // run at startup
    sqlite3_config(SQLITE_CONFIG_LOG, print_error, nullptr);
} } sqlite_log;

} // anonymous namespace

/*****************************************************/
SqliteDatabase::SqliteDatabase(const std::string& path) :
    mDebug(__func__,this)
{
    MDBG_INFO("(path:" << path << ")");

    const int rc = sqlite3_open(path.c_str(), &mDatabase);
    check_rc(rc, [this]{ sqlite3_close(mDatabase); });

    RowList rows; query("PRAGMA foreign_keys = true",{},rows);
}

/*****************************************************/
SqliteDatabase::~SqliteDatabase()
{
    MDBG_INFO("()");

    print_rc(sqlite3_close(mDatabase));
}

/*****************************************************/
std::string SqliteDatabase::print_rc(int rc) const
{
    if (rc != SQLITE_OK)
    {
        std::ostringstream msg;
        const std::string errmsg(sqlite3_errmsg(mDatabase)); // get first

        msg << sqlite3_errstr(rc) << " (" << errmsg << ")";
        MDBG_ERROR("... error:" << rc << " (" << msg.str() << ")");
        return msg.str();
    }
    else return {}; // no error
}

/*****************************************************/
void SqliteDatabase::check_rc(int rc, const VoidFunc& func) const
{
    if (rc != SQLITE_OK)
    {
        const std::string errmsg { print_rc(rc) };
        if (func) func(); // custom handler
        throw Exception(errmsg);
    }
}

/*****************************************************/
size_t SqliteDatabase::query(const std::string& sql, const MixedParams& params, RowList& rows)
{
    MDBG_INFO("(sql:" << sql << ")");
    const std::lock_guard<std::mutex> lock(mMutex);

    sqlite3_stmt* stmt { nullptr };
    const int sqllen { static_cast<int>(sql.size())+1 };
    check_rc(sqlite3_prepare_v2(mDatabase, sql.c_str(), sqllen, &stmt, nullptr));
    if (stmt == nullptr) throw Exception("statement is nullptr");

    for (const MixedParams::value_type& param : params)
    {
        const int idx { sqlite3_bind_parameter_index(stmt, param.first.c_str()) };
        check_rc(param.second.bind(*stmt, idx));
    }

    rows.clear();
    int rc { -1 };
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        Row& row { rows.emplace_back() };
        for (int idx = 0; idx < sqlite3_column_count(stmt); ++idx)
        {
            row.emplace(std::make_pair(sqlite3_column_name(stmt,idx), 
                MixedOutput(*sqlite3_column_value(stmt,idx))));
        }
    }
    MDBG_INFO("... #rows returned:" << rows.size());

    if (rc == SQLITE_DONE) rc = SQLITE_OK;
    check_rc(rc, [stmt]{ sqlite3_finalize(stmt); });
    check_rc(sqlite3_finalize(stmt));

    // count of rows matched, only valid for INSERT, UPDATE, DELETE
    return static_cast<size_t>(sqlite3_changes(mDatabase));
}

} // namespace Database
} // namespace Andromeda

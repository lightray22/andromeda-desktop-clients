
#include <filesystem>
#include <mutex>
#include <sstream>
#include <utility>

#include <sqlite3.h>

#include "SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {

namespace { // anonymous

Debug sDebug("libsqlite3",nullptr); // NOLINT(cert-err58-cpp)

std::mutex sLogConfigMutex;
bool sLogConfigured = false;

void print_error(void* pArg, int errCode, const char* zMsg){
    SDBG_ERROR("... code:" << errCode << " (" << zMsg << ")");
}

} // anonymous namespace

/*****************************************************/
SqliteDatabase::SqliteDatabase(const std::string& path) :
    mDebug(__func__,this)
{
    MDBG_INFO("(path:" << path << ")");

    { // lock scope
        const std::lock_guard<std::mutex> logLock(sLogConfigMutex);
        if (!sLogConfigured)
        {
            MDBG_INFO("... sqlite3_config(SQLITE_CONFIG_LOG)");
            sqlite3_config(SQLITE_CONFIG_LOG, print_error, nullptr);
            sLogConfigured = true;
        }
    }

    const int rc = sqlite3_open(path.c_str(), &mDatabase);
    check_rc(rc, [this]{ sqlite3_close(mDatabase); });

    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    query("PRAGMA foreign_keys = true",{},lock);
    query("PRAGMA trusted_schema = false",{},lock);
    { RowList rows; query("PRAGMA journal_mode = TRUNCATE",{},rows,lock); } // ignore rows

    if (mDebug.GetLevel() >= Debug::Level::INFO)
    {
        RowList rows; query("PRAGMA integrity_check",{},rows,lock);
        for (const Row& row : rows)
        {
            const char* err { nullptr }; row.begin()->second.get_to(err);
            MDBG_INFO("... integrity check: " << err);
        }
    }

    const int version { getVersion() };
    MDBG_INFO("... version: " << version);
}

/*****************************************************/
SqliteDatabase::~SqliteDatabase() // NOLINT(bugprone-exception-escape)
{
    MDBG_INFO("()");

    if (mDatabase == nullptr) return; // unit test

    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    
    if (sqlite3_get_autocommit(mDatabase) == 0) // inTransaction
    {
        MDBG_ERROR("... still in transaction! rolling back...");

        try { query("ROLLBACK TRANSACTION",{},lock); }
        catch (const DatabaseException& e) { 
            MDBG_ERROR("... rollback error:" << e.what()); }
    }

    try { query("PRAGMA optimize",{},lock); }
    catch (const DatabaseException& e) { 
        MDBG_ERROR("... optimize error:" << e.what()); }
    
    print_rc(sqlite3_close(mDatabase)); // nothrow
}

/*****************************************************/
void SqliteDatabase::check_rc(int rc, const VoidFunc& func) const
{
    if (rc != SQLITE_OK)
    {
        const std::string errmsg { print_rc(rc) };
        if (func) func(); // custom handler
        throw SqliteException(errmsg);
    }
}

/*****************************************************/
std::string SqliteDatabase::print_rc(int rc) const
{
    if (rc != SQLITE_OK)
    {
        const std::string errmsg(sqlite3_errmsg(mDatabase)); // get first

        std::ostringstream msg;
        msg << sqlite3_errstr(rc) << " (" << errmsg << ")";
        MDBG_ERROR("... error:" << rc << " (" << msg.str() << ")");
        return msg.str();
    }
    else return {}; // no error
}

/*****************************************************/
size_t SqliteDatabase::query(const std::string& sql, const MixedParams& params)
{
    RowList rows; const size_t retval { query(sql, params, rows) };
    if (!rows.empty()) throw SqliteException("non-empty rows!");
    else return retval;
}

/*****************************************************/
size_t SqliteDatabase::query(const std::string& sql, const MixedParams& params, const UniqueLock& lock)
{
    RowList rows; const size_t retval { query(sql, params, rows, lock) };
    if (!rows.empty()) throw SqliteException("non-empty rows!");
    else return retval;
}

/*****************************************************/
size_t SqliteDatabase::query(const std::string& sql, const MixedParams& params, RowList& rows)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return query(sql, params, rows, lock);
}

/*****************************************************/
size_t SqliteDatabase::query(const std::string& sql, const MixedParams& params, RowList& rows, const UniqueLock& lock)
{
    MDBG_INFO("(sql:" << sql << ")");

    sqlite3_stmt* stmt { nullptr };
    const int sqllen { static_cast<int>(sql.size())+1 };
    check_rc(sqlite3_prepare_v2(mDatabase, sql.c_str(), sqllen, &stmt, nullptr));
    if (stmt == nullptr) throw SqliteException("statement is nullptr");

    for (const MixedParams::value_type& param : params)
    {
        const int idx { sqlite3_bind_parameter_index(stmt, param.first.c_str()) };
        check_rc(param.second.bind(*stmt, idx), [stmt]{ sqlite3_finalize(stmt); });
    }

    int rc { -1 };
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        Row& row { rows.emplace_back() };
        for (int idx = 0; idx < sqlite3_column_count(stmt); ++idx)
        {
            row.emplace(sqlite3_column_name(stmt,idx), 
                *sqlite3_column_value(stmt,idx));
        }
    }
    MDBG_INFO("... #rows returned:" << rows.size());

    if (rc == SQLITE_DONE) rc = SQLITE_OK; // both ok
    check_rc(rc, [stmt]{ sqlite3_finalize(stmt); });
    check_rc(sqlite3_finalize(stmt));

    // count of rows matched, only valid for INSERT, UPDATE, DELETE
    return static_cast<size_t>(sqlite3_changes(mDatabase));
}

/*****************************************************/
void SqliteDatabase::transaction(const std::function<void()>& func)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (!mDatabase) { func(); return; } // unit test

    if (sqlite3_get_autocommit(mDatabase) == 0) // inTransaction
        throw TransactionException();
    else query("BEGIN TRANSACTION",{},lock);

    try { func(); query("COMMIT TRANSACTION",{},lock); }
    catch (const DatabaseException& e)
    {
        query("ROLLBACK TRANSACTION",{},lock);
        throw; // rethrow
    }
}

/*****************************************************/
void SqliteDatabase::beginTransaction()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (!mDatabase) return; // unit test

    if (sqlite3_get_autocommit(mDatabase) == 0) // inTransaction
        throw TransactionException();
    else query("BEGIN TRANSACTION",{},lock);
}

/*****************************************************/
void SqliteDatabase::rollback()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (!mDatabase) return; // unit test

    if (sqlite3_get_autocommit(mDatabase) != 0) // !inTransaction
        throw TransactionException();
    else query("ROLLBACK TRANSACTION",{},lock);
}

/*****************************************************/
void SqliteDatabase::commit()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (!mDatabase) return; // unit test

    if (sqlite3_get_autocommit(mDatabase) != 0) // !inTransaction
        throw TransactionException();
    else query("COMMIT TRANSACTION",{},lock);
}

/*****************************************************/
int SqliteDatabase::getVersion()
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    RowList rows; query("PRAGMA user_version",{},rows,lock);
    return rows.begin()->begin()->second.get<int>();
}

/*****************************************************/
void SqliteDatabase::setVersion(int version)
{
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    query("PRAGMA user_version = "+std::to_string(version),{});
}

} // namespace Database
} // namespace Andromeda

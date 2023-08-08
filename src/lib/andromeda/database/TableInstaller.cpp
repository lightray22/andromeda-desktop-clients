
#include "SqliteDatabase.hpp"
#include "TableInstaller.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
TableInstaller::TableInstaller(ObjectDatabase& database) :
    mDebug(__func__,this), mDatabase(database)
{
    MDBG_INFO("()");
    
    SqliteDatabase& sqlDb { database.getInternal() };

    const int dbVer { sqlDb.getVersion() };
    const int codeVer { VersionEntry::GetTableVersion() };
    if (dbVer == 0)
    {
        MDBG_INFO("... install version table!");

        const TableBuilder tb { VersionEntry::GetTableInstall() };
        for (const std::string& query : tb.GetQueries())
            sqlDb.query(query,{});

        sqlDb.setVersion(codeVer);
    }
    else if (dbVer > codeVer)
        throw TableVersionException("version table");
    else for (int newVer = dbVer+1; newVer <= codeVer; ++newVer)
    {
        MDBG_INFO("... upgrade version table to version " << std::to_string(newVer));

        const TableBuilder tb { VersionEntry::GetTableUpgrade(newVer) };
        for (const std::string& query : tb.GetQueries())
            sqlDb.query(query,{});

        sqlDb.setVersion(newVer);
    }
}

} // namespace Database
} // namespace Andromeda

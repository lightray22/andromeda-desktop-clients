#ifndef LIBA2_TABLEINSTALLER_H_
#define LIBA2_TABLEINSTALLER_H_

#include "BaseObject.hpp"
#include "ObjectDatabase.hpp"
#include "TableBuilder.hpp"
#include "VersionEntry.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Database {

/** Handles installing and upgrading BaseObject database tables */
class TableInstaller
{
public:

    /** Exception indicating that the table version is too new */
    class TableVersionException : public DatabaseException { public:
        explicit TableVersionException(const std::string& tableName) : 
            DatabaseException("table too new: "+tableName) {}; };

    /** 
     * Construct, and create/upgrade the version table
     * @throws DatabaseException if any myriad of things go wrong
     */
    explicit TableInstaller(ObjectDatabase& database);

    /* The BaseObject MUST define these static functions:
    [[nodiscard]] inline static int GetTableVersion(); // return the newest version
    static TableBuilder GetTableInstall(); // return the TableBuilder for installing the table
    static TableBuilder GetTableUpgrade(int newVersion); // return the TableBuilder to upgrade from newVersion-1 to newVersion
    */

    /** 
     * Install or upgrade the given BaseObject T class
     * REQUIRES GetTableVersion(), GetTableInstall(), GetTableUpgrade(int)
     * @throws DatabaseException if any myriad of things go wrong
     */
    template<typename T>
    void InstallTable()
    {
        SqliteDatabase& sqlDb { mDatabase.getInternal() };

        const std::string tableName { mDatabase.GetClassTableName(T::GetClassNameS()) };
        VersionEntry* verEntry { VersionEntry::TryLoadByTable(mDatabase, tableName) };

        const int codeVer { T::GetTableVersion() };
        if (verEntry == nullptr)
        {
            MDBG_INFO("... install " << tableName);
            
            const TableBuilder tb { T::GetTableInstall() };
            for (const std::string& query : tb.GetQueries())
                sqlDb.query(query,{});

            VersionEntry::Create(mDatabase, tableName, codeVer).Save();
        } 
        else if (verEntry->getVersion() > codeVer)
            throw TableVersionException(tableName);
        else for (int newVer = verEntry->getVersion()+1; newVer <= codeVer; ++newVer)
        {
            MDBG_INFO("... upgrade " << tableName << " to version " << std::to_string(newVer));

            const TableBuilder tb { T::GetTableUpgrade(newVer) };
            for (const std::string& query : tb.GetQueries())
                sqlDb.query(query,{});

            verEntry->setVersion(newVer); verEntry->Save();
        }
    }

private:

    mutable Andromeda::Debug mDebug;

    ObjectDatabase& mDatabase;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_TABLEINSTALLER_H_

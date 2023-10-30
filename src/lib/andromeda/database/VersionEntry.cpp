
#include "ObjectDatabase.hpp"
#include "VersionEntry.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
VersionEntry::VersionEntry(ObjectDatabase& database, const MixedParams& data, bool created) :
    BaseObject(database),
    mTable("tableName",*this),
    mVersion("version",*this)
{
    RegisterFields({&mTable, &mVersion});
    InitializeFields(data, created);
}

/*****************************************************/
TableBuilder VersionEntry::GetTableInstall()
{
    TableBuilder tb { TableBuilder::For<VersionEntry>() };
    tb.AddColumn("id","varchar(16)",false).SetPrimary("id")
      .AddColumn("tableName","varchar(255)",false).AddUnique("tableName")
      .AddColumn("version","integer",false);
    return tb;
}

/*****************************************************/
TableBuilder VersionEntry::GetTableUpgrade(int newVersion)
{
    return TableBuilder::For<VersionEntry>(); // empty
}

/*****************************************************/
VersionEntry& VersionEntry::Create(ObjectDatabase& db, const std::string& tableName, int version)
{
    VersionEntry& obj { db.CreateObject<VersionEntry>() };
    obj.mTable = tableName;
    obj.mVersion = version;
    return obj;
}

/*****************************************************/
VersionEntry* VersionEntry::TryLoadByTable(ObjectDatabase& db, const std::string& tableName)
{
    QueryBuilder q; q.Where(q.Equals("tableName",tableName));
    return db.TryLoadUniqueByQuery<VersionEntry>(q);
}

} // namespace Database
} // namespace Andromeda

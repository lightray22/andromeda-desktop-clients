
#include "SessionStore.hpp"

#include "andromeda/database/MixedValue.hpp"
using Andromeda::Database::MixedParams;
#include "andromeda/database/ObjectDatabase.hpp"
using Andromeda::Database::ObjectDatabase;
#include "andromeda/database/TableBuilder.hpp"
using Andromeda::Database::TableBuilder;

namespace Andromeda {
namespace Backend {

/*****************************************************/
SessionStore::SessionStore(ObjectDatabase& database, const MixedParams& data) :
    BaseObject(database),
    mServerUrl("serverUrl",*this),
    mAccountID("accountID",*this),
    mSessionID("sessionID",*this),
    mSessionKey("sessionKey",*this)
{
    RegisterFields({&mServerUrl, &mAccountID, &mSessionID, &mSessionKey});
    InitializeFields(data);
}

/*****************************************************/
TableBuilder SessionStore::GetTableInstall()
{
    TableBuilder tb { TableBuilder::For<SessionStore>() };
    tb.AddColumn("id","varchar(12)",false).SetPrimary("id")
      .AddColumn("serverUrl","text",false)
      .AddColumn("accountID","char(12)",false).AddUnique("accountID")
      .AddColumn("sessionID","char(12)",true)
      .AddColumn("sessionKey","char(32)",true);
    return tb;
}

/*****************************************************/
TableBuilder SessionStore::GetTableUpgrade(int newVersion)
{
    return TableBuilder::For<SessionStore>(); // empty
}

/*****************************************************/
SessionStore& SessionStore::Create(ObjectDatabase& db, const std::string& serverUrl, const std::string& accountID)
{
    SessionStore& obj { db.CreateObject<SessionStore>() };
    obj.mServerUrl = serverUrl;
    obj.mAccountID = accountID;
    return obj;
}

/*****************************************************/
std::list<SessionStore*> SessionStore::LoadAll(ObjectDatabase& db) // cppcheck-suppress constParameter
{
    return db.LoadObjectsByQuery<SessionStore>({}); // empty WHERE
}

/*****************************************************/
void SessionStore::SetSession(std::nullptr_t)
{
    mSessionID = nullptr;
    mSessionKey = nullptr;
}

/*****************************************************/
void SessionStore::SetSession(const std::string& sessionID, const std::string& sessionKey)
{
    mSessionID = sessionID;
    mSessionKey = sessionKey;
}

} // namespace Backend
} // namespace Andromeda

#ifndef LIBA2_SESSIONSTORE_H_
#define LIBA2_SESSIONSTORE_H_

#include <cstddef>
#include <list>
#include <string>

#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/TableBuilder.hpp"
#include "andromeda/database/fieldtypes/ScalarType.hpp"

namespace Andromeda {
namespace Backend {

/** Stores an account and session in the database */
class SessionStore : public Database::BaseObject
{
public:

    // BaseObject functions
    BASEOBJECT_NAME(SessionStore, "Andromeda\\Database\\SessionStore")
    SessionStore(Database::ObjectDatabase& database, const Database::MixedParams& data);

    // TableInstaller functions
    [[nodiscard]] inline static int GetTableVersion() { return 1; }
    static Database::TableBuilder GetTableInstall();
    static Database::TableBuilder GetTableUpgrade(int newVersion);

    /** Create a new session store for the given server URL and accountID (sessionID and key are null) */
    static SessionStore& Create(Database::ObjectDatabase& db, const std::string& serverUrl, const std::string& accountID);

    /** 
     * Loads a list of non-null pointers to all saved sessions 
     * @throws DatabaseException
     */
    static std::list<SessionStore*> LoadAll(Database::ObjectDatabase& db);

    /** Returns the server URL this session is for */
    inline const std::string& GetServerUrl() const { return mServerUrl.GetValue(); }
    /** Returns the account ID this session is for */
    inline const std::string& GetAccountID() const { return mAccountID.GetValue(); }
    /** Returns the session ID or nullptr if none */
    inline const std::string* GetSessionID() const { return mSessionID.TryGetValue(); }
    /** Returns the session Key or nullptr if none */
    inline const std::string* GetSessionKey() const { return mSessionKey.TryGetValue(); }

    /** Set the session ID and key to nullptr */
    void SetSession(std::nullptr_t);
    /** Set the session ID and key to the given values */
    void SetSession(const std::string& sessionID, const std::string& sessionKey);

private:

    Database::FieldTypes::ScalarType<std::string> mServerUrl;
    Database::FieldTypes::ScalarType<std::string> mAccountID;
    Database::FieldTypes::NullScalarType<std::string> mSessionID;
    Database::FieldTypes::NullScalarType<std::string> mSessionKey;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_SESSIONSTORE_H_

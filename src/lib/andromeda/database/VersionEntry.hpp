#ifndef LIBA2_VERSIONENTRY_H_
#define LIBA2_VERSIONENTRY_H_

#include "BaseObject.hpp"
#include "TableBuilder.hpp"
#include "fieldtypes/ScalarType.hpp"

namespace Andromeda {
namespace Database {

class ObjectDatabase;

/** 
 * Stores the version of a BaseObject's table
 * Lets TableInstaller determine when to install/upgrade tables
 */
class VersionEntry : public BaseObject
{
public:
    // BaseObject functions
    BASEOBJECT_NAME(VersionEntry, "Andromeda\\Database\\VersionEntry")
    VersionEntry(ObjectDatabase& database, const MixedParams& data);

    // TableInstaller functions
    [[nodiscard]] inline static int GetTableVersion() { return 1; }
    static TableBuilder GetTableInstall();
    static TableBuilder GetTableUpgrade(int newVersion);

    /** Create a new VersionEntry with the given DB table name and version */
    static VersionEntry& Create(ObjectDatabase& db, const std::string& tableName, int version);

    /** 
     * Return a pointer to the VersionEntry for tableName if it exists, else nullptr
     * @throws DatabaseException
     */
    static VersionEntry* TryLoadByTable(ObjectDatabase& db, const std::string& tableName);

    /** Returns the current table version number for this entry */
    inline int getVersion() const { return mVersion.GetValue(); }
    /** Sets the current table version numbe entry */
    inline void setVersion(int newVer) { mVersion = newVer; }

private:

    FieldTypes::ScalarType<std::string> mTable;
    FieldTypes::ScalarType<int> mVersion;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_VERSIONENTRY_H_

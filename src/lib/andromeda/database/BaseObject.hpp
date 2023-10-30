#ifndef LIBA2_BASEOBJECT_H_
#define LIBA2_BASEOBJECT_H_

#include <list>
#include <map>
#include <string>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "fieldtypes/BaseField.hpp"
#include "fieldtypes/ScalarType.hpp"

namespace Andromeda {
namespace Database {

class MixedParams;
class ObjectDatabase;


/**
 * The base class for objects that can be saved/loaded from the database.
 * 
 * Mostly ported from the server's PHP implementation, but simplified
 * without support for object inheritance or splitting tables
 * 
 * NOT THREAD SAFE!
 */
class BaseObject
{
public:

    /** Return the unique class name string of this BaseObject */
    virtual const char* GetClassName() const = 0;
    #define BASEOBJECT_NAME(T, name) \
        inline static const char* GetClassNameS() { return name; } \
        inline const char* GetClassName() const override { return T::GetClassNameS(); }

    virtual ~BaseObject() = default;
    DELETE_COPY(BaseObject)
    DELETE_MOVE(BaseObject)

    /** Returns the object's associated database */
    inline ObjectDatabase& GetDatabase() const { return mDatabase; }

    /** Returns this object's base-unique ID */
    const std::string& ID() const;

    /** Return the object type and ID as a string for debug */
    explicit operator std::string() const;

    inline bool operator==(const BaseObject& cmp) const { return &cmp == this; }

    /** Gives modified fields to the database to UPDATE or INSERT */
    virtual void Save();

    using FieldList = std::list<FieldTypes::BaseField*>;
    // ordered map for unit test checking queries
    using FieldMap = std::map<std::string, FieldTypes::BaseField&>;

protected:

    #define OBJDBG_INFO(strfunc) MDBG_INFO("(" << ID() << ")" << strfunc)
    #define OBJDBG_ERROR(strfunc) MDBG_ERROR("(" << ID() << ")" << strfunc)

    friend class ObjectDatabase;

    /** 
     * Construct an object from a database reference
     * @param database associated database
     * @param created true if the object is newly created
    */
    explicit BaseObject(ObjectDatabase& database);

    /**
     * Notifies this object that the DB is about to delete it - INTERNAL ONLY
     * This is ONLY run when deleting a single object directly, not deleting by query!
     * Child classes can extend this if they need extra on-delete logic
     */
    virtual void NotifyPreDeleted() { }

    /**
     * Notifies this object that the DB has deleted it - INTERNAL ONLY
     * Child classes can extend this if they need extra on-delete logic
     */
    virtual void NotifyPostDeleted() { }

    /**
     * Registers fields for the object so the DB can save/load objects
     * Child classes MUST call this in their constructor to initialize
     * @param list list of fields for this object
     */
    void RegisterFields(const FieldList& list);

    /**
     * Initialize all fields from database data
     * @param data map of fields from the database
     * @param created true if the object is newly created
     */
    void InitializeFields(const MixedParams& data, bool created);

    /** Returns true if this object has a modified field */
    bool isModified() const;

    /** Set all fields as unmodified */
    void SetUnmodified();

    /** object database reference */
    ObjectDatabase& mDatabase;

private:

    mutable Debug mDebug;

    FieldTypes::ScalarType<std::string> mIdField;

    FieldMap mFields;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_BASEOBJECT_H_

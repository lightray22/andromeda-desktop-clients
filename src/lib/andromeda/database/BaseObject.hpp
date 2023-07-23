#ifndef LIBA2_BASEOBJECT_H_
#define LIBA2_BASEOBJECT_H_

#include <list>
#include <string>
#include <unordered_map>

#include "FieldTypes.hpp"
#include "andromeda/Utilities.hpp"

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

    #define BASEOBJECT_CONSTRUCT(T) \
        T(ObjectDatabase& database, const MixedParams& data): \
            BaseObject(database),

    #define BASEOBJECT_REGISTER(...) \
        RegisterFields({__VA_ARGS__}); \
        InitializeFields(data);

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
    using FieldMap = std::unordered_map<std::string, FieldTypes::BaseField&>;

protected:

    friend class ObjectDatabase;

    /** 
     * Construct an object from a database reference
     * @param database associated database
    */
    explicit BaseObject(ObjectDatabase& database);

    /**
     * Notifies this object that the DB has deleted it - INTERNAL ONLY
     * Child classes can extend this if they need extra on-delete logic
     */
    virtual void NotifyDeleted() { }

    /**
     * Registers fields for the object so the DB can save/load objects
     * Child classes MUST call this in their constructor to initialize
     * @param list list of fields for this object
     */
    void RegisterFields(const FieldList& list);

    /**
     * Initialize all fields from database data
     * @param data map of fields from the database
     */
    void InitializeFields(const MixedParams& data);

    /** Sets the ID field on a newly created object */
    void InitializeID(size_t len = 16);

    /** Returns true if this object has a modified field */
    bool isModified() const;

    /** object database reference */
    ObjectDatabase& mDatabase;

private:

    FieldTypes::ScalarType<std::string> mIdField;

    FieldMap mFields;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_BASEOBJECT_H_

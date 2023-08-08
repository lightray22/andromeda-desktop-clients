#ifndef LIBA2_OBJECTREF_H_
#define LIBA2_OBJECTREF_H_

#include <cstddef>
#include <string>

#include "BaseField.hpp"
#include "andromeda/database/ObjectDatabase.hpp"
#include "andromeda/database/QueryBuilder.hpp"

namespace Andromeda {
namespace Database {
class BaseObject;

namespace FieldTypes {

/**
 * A field holding a possibly-null object reference
 * @tparam T BaseObject type
 */
template<typename T>
class NullObjectRef : public BaseField
{
public:
    /** Construct with a null default value */
    using BaseField::BaseField;

    /** Exception indicating a foreign key reference failed */
    class ForeignKeyException : public DatabaseException { public:
        explicit ForeignKeyException(const std::string& className) :
            DatabaseException("Foreign key missing: "+className) {}; };

    void InitDBValue(const MixedValue& value) override
    {
        mDelta = 0;
        if (!value.is_null())
            value.get_to(mObjId);
    }

    [[nodiscard]] MixedValue GetDBValue() const override
    {
        if (mObjId.empty()) return MixedValue(nullptr);
        else return MixedValue(mObjId); // NOLINT(google-readability-casting) ???
    }

    /** Returns true if the object is NULL */
    [[nodiscard]] inline bool is_null() const { return mObjId.empty(); }

    /** 
     * Returns a pointer to the object or nullptr if NULL
     * @throws ForeignKeyException if the load unique query fails
     */
    [[nodiscard]] T* TryGetObject() const
    {
        if (mObjId.empty()) return nullptr;

        if (mObjPtr == nullptr)
        {
            QueryBuilder q; q.Where(q.Equals("id",mObjId));
            mObjPtr = mDatabase.TryLoadUniqueByQuery<T>(q);
            if (mObjPtr == nullptr) throw ForeignKeyException(T::GetClassNameS());
        }
        return mObjPtr;
    }

    /** Returns a pointer to the object or nullptr if NULL */
    inline explicit operator T*() const { return TryGetObject(); }
    /** Returns a reference to the object (assumes not NULL) */
    inline T& operator*() const { return *TryGetObject(); }
    /** Return true if NULL */
    inline bool operator==(std::nullptr_t) const { return is_null(); }
    /** Return true if NOT NULL */
    inline bool operator!=(std::nullptr_t) const { return !(*this==nullptr); }

    /**
     * Sets the field to the given object
     * @return true if the reference is modified
     */
    bool SetObject(T& object)
    {
        mObjPtr = &object;
        if (mObjId == object.ID()) return false;

        notifyModified();
        mObjId = object.ID();
        ++mDelta;
        return true;
    }

    /**
     * Sets the field to NULL
     * @return true if the object is modified
     */
    bool SetObject(std::nullptr_t)
    {
        if (mObjId.empty()) return false;

        notifyModified();
        mObjId.clear();
        mObjPtr = nullptr;
        ++mDelta;
        return true;
    }

    /** Sets the field to the given object */
    inline NullObjectRef& operator=(T& object) { SetObject(object); return *this; }
    /** Sets the field to NULL */
    inline NullObjectRef& operator=(std::nullptr_t) { SetObject(nullptr); return *this; }

protected:
    std::string mObjId;
    mutable T* mObjPtr { nullptr }; // cache
};

/**
 * A field holding a non-null object reference
 * @tparam T BaseObject type
 */
template<typename T>
class ObjectRef : public BaseField
{
public:
    /** Construct uninitialized */
    using BaseField::BaseField;

    /** Exception indicating a foreign key reference failed */
    class ForeignKeyException : public DatabaseException { public:
        explicit ForeignKeyException(const std::string& className) :
            DatabaseException("Foreign key missing: "+className) {}; };

    /** @throws DBValueNullException if the given DB value is null */
    void InitDBValue(const MixedValue& value) override
    {
        if (value.is_null()) throw DBValueNullException(mName);

        mDelta = 0;
        value.get_to(mObjId);
    }

    /** @throws UninitializedException if the field is not initialized */
    [[nodiscard]] MixedValue GetDBValue() const override
    {
        if (mObjId.empty()) throw UninitializedException(mName);
        else return MixedValue(mObjId); // NOLINT(google-readability-casting) ???
    }

    /** Returns true if the object is initialized */
    [[nodiscard]] bool isInitialized() const { return !mObjId.empty(); }

    /** 
     * Returns the field's object
     * @throws UninitializedException if the field is not initialized
     * @throws ForeignKeyException if the load unique query fails
     */
    [[nodiscard]] T& GetObject() const
    {
        if (mObjId.empty()) throw UninitializedException(mName);

        if (mObjPtr == nullptr)
        {
            QueryBuilder q; q.Where(q.Equals("id",mObjId));
            mObjPtr = mDatabase.TryLoadUniqueByQuery<T>(q);
            if (mObjPtr == nullptr) throw ForeignKeyException(T::GetClassNameS());
        }
        return *mObjPtr;
    }

    /** Returns the field's object */
    inline explicit operator T&() const { return GetObject(); }
    /** Return true if equal to the given object */
    inline bool operator==(const T& object) const { return GetObject() == object; }
    /** Return true if not equal to the given object */
    inline bool operator!=(const T& rhs) const { return !(*this==rhs); }

    /**
     * Sets the field to the given object
     * @return true if the reference is modified
     */
    bool SetObject(T& object)
    {
        mObjPtr = &object;
        if (mObjId == object.ID()) return false;

        notifyModified();
        mObjId = object.ID();
        ++mDelta;
        return true;
    }

    /** Sets the field to the given object */
    inline ObjectRef& operator=(T& object) { SetObject(object); return *this; }

protected:
    std::string mObjId;
    mutable T* mObjPtr { nullptr }; // cache
};

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

#endif // LIBA2_OBJECTREF_H_

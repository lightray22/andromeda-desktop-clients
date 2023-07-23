#ifndef LIBA2_FIELDTYPES_H_
#define LIBA2_FIELDTYPES_H_

#include <cstddef>
#include <string>

#include "MixedValue.hpp"
#include "andromeda/BaseException.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Database {

class BaseObject;
class ObjectDatabase;

namespace FieldTypes {

/** 
 * Base class representing a database column ("field") 
 * Mostly ported from the server's PHP implementation, scalar types are combined
 */
class BaseField
{
public:
    virtual ~BaseField() = default;
    DELETE_COPY(BaseField)
    DELETE_MOVE(BaseField)

    /** Exception indicating an uninitialized non-null field was accessed */
    class UninitializedException : public BaseException { public:
        explicit UninitializedException(const std::string& name) :
            BaseException("Uninitialized Field: "+name) {}; };

    /** @return string field name in the DB */
    [[nodiscard]] inline const char* GetName() const { return mName; }

    /** @return int number of times modified */
    [[nodiscard]] inline size_t GetDelta() const { return mDelta; }

    /** @return bool true if was modified from DB */
    [[nodiscard]] inline bool isModified() const { return mDelta > 0; }

    /** Initializes the field's value from the DB */
    virtual void InitDBValue(const MixedValue& value) = 0;

    /** Returns the field's database input value */
    [[nodiscard]] virtual MixedValue GetDBValue() const = 0;

    /** Returns true if the value is a relative increment, not absolute */
    [[nodiscard]] virtual bool UseDBIncrement() const { return false; }
    
    /** Resets this field's delta */
    inline void SetUnmodified() { mDelta = 0; }

protected:

    BaseField(const char* name, BaseObject& parent, size_t delta = 0);

    /** notifies the database our parent is modified */
    void notifyModified();

    /** name of the field/column in the DB */
    const char* mName;

    /** number of times the field is modified */
    size_t mDelta;

    /** parent object reference */
    BaseObject& mParent;

    /** database reference */
    ObjectDatabase& mDatabase;
};

template<typename T>
class NullScalarType : public BaseField
{
public:
    /** Construct with a null default value */
    NullScalarType(const char* name, BaseObject& parent) :
        BaseField(name, parent) { }

    /** Construct with a non-null default value and set dirty */
    NullScalarType(const char* name, BaseObject& parent, const T& defaultt) :
        BaseField(name, parent, 1), 
        mTempNull(false), mTempValue(defaultt),
        mRealNull(false), mRealValue(defaultt) { }

    void InitDBValue(const MixedValue& value) override
    {
        mDelta = 0;
        mTempNull = value.is_null();
        mRealNull = value.is_null();
        if (!value.is_null())
        {
            value.get_to(mTempValue);
            value.get_to(mRealValue);
        }
    }

    [[nodiscard]] MixedValue GetDBValue() const override
    {
        if (mRealNull) return MixedValue(nullptr);
        else return MixedValue(mRealValue);
    }

    /** Returns true if the value is NULL */
    [[nodiscard]] inline bool is_null() const { return mTempNull; }

    /** 
     * Returns a pointer to the value or nullptr if NULL
     * @param allowTemp if false, only get the "real" (not temp) value
     */
    [[nodiscard]] const T* TryGetValue(bool allowTemp = true) const
    {
        if (allowTemp)
            return mTempNull ? nullptr : &mTempValue;
        else return mRealNull ? nullptr : &mRealValue;
    }

    /** Returns a pointer to the value or nullptr if NULL */
    inline explicit operator const T*() const { return TryGetValue(); }

    /** Returns a reference to the value (assumes not NULL) */
    inline const T& operator*() const { return *TryGetValue(); }

    /** Return true if NULL */
    inline bool operator==(std::nullptr_t) const { return mTempNull; }

    /** Return true if NOT NULL */
    inline bool operator!=(std::nullptr_t) const { return !(*this==nullptr); }

    /**
     * Sets the field to the given value
     * @param isTemp if true, this is temporary (don't save to DB)
     * @return true if the value is modified
     */
    bool SetValue(const T& value, bool isTemp = false)
    {
        mTempNull = false;
        mTempValue = value;

        if (!isTemp && mRealValue != value)
        {
            notifyModified();
            mRealNull = false;
            mRealValue = value;
            ++mDelta; return true;
        }

        return false;
    }

    /**
     * Sets the field to NULL
     * @param isTemp if true, this is temporary (don't save to DB)
     * @return true if the value is modified
     */
    bool SetValue(std::nullptr_t, bool isTemp = false)
    {
        mTempNull = true;

        if (!isTemp && !mRealNull)
        {
            notifyModified();
            mRealNull = true;
            ++mDelta; return true;
        }

        return false;
    }

    /** Sets the field to the given value */
    inline NullScalarType& operator=(const T& value) { SetValue(value); return *this; }

    /** Sets the field to NULL */
    inline NullScalarType& operator=(std::nullptr_t) { SetValue(nullptr); return *this; }

protected:
    bool mTempNull { true };
    T mTempValue {};
    bool mRealNull { true };
    T mRealValue {};
};

template<typename T>
class ScalarType : public BaseField
{
public:
    /** Construct with a null default value (uninitialized) */
    ScalarType(const char* name, BaseObject& parent) :
        BaseField(name, parent) { }

    /** Construct with a non-null default value and set dirty */
    ScalarType(const char* name, BaseObject& parent, const T& defaultt) :
        BaseField(name, parent, 1), 
        mTempInitd(true), mTempValue(defaultt),
        mRealInitd(true), mRealValue(defaultt) { }

    void InitDBValue(const MixedValue& value) override
    {
        mDelta = 0;
        mTempInitd = true;
        mRealInitd = true;
        value.get_to(mTempValue);
        value.get_to(mRealValue);
    }

    /** @throws UninitializedException if the field is not initialized */
    [[nodiscard]] MixedValue GetDBValue() const override
    {
        if (!mRealInitd) throw UninitializedException(mName);

        return MixedValue(mRealValue);
    }

    /** Returns true if the value is initialized */
    [[nodiscard]] bool isInitialized(bool allowTemp = true) const
    { 
        return allowTemp ? mTempInitd : mRealInitd;
    }

    /** 
     * Returns the field's value
     * @param allowTemp if false, only get the "real" (not temp) value
     * @throws UninitializedException if the field is not initialized
     */
    [[nodiscard]] const T& GetValue(bool allowTemp = true) const
    {
        if (!(allowTemp ? mTempInitd : mRealInitd))
            throw UninitializedException(mName);

        if (allowTemp) return mTempValue; else return mRealValue;
    }

    /** Returns the field's value */
    inline explicit operator const T&() const { return GetValue(); }

    /** Return true if equal to the given value */
    inline bool operator==(const T& value) const { return GetValue() == value; }

    /** Return true if not equal to the given value */
    inline bool operator!=(const T& rhs) const { return !(*this==rhs); }

    /**
     * Sets the field to the given value
     * @param isTemp if true, this is temporary (don't save to DB)
     * @return true if the value is modified
     */
    bool SetValue(const T& value, bool isTemp = false)
    {
        mTempInitd = true;
        mTempValue = value;

        if (!isTemp && (!mRealInitd || mRealValue != value))
        {
            notifyModified();
            mRealInitd = true;
            mRealValue = value;
            ++mDelta; return true;
        }

        return false;
    }

    /** Sets the field to the given value */
    inline ScalarType& operator=(const T& value) { SetValue(value); return *this; }

protected:
    bool mTempInitd { false };
    T mTempValue {};
    bool mRealInitd { false };
    T mRealValue {};
};

// TODO !! add other types - counter, JSON array, ObjectRef
// will definitely want JSON/ObjectRef in other files... JSON will include nlohmann
// and ObjectRef will need to include ObjectDatabase.hpp

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

#endif // LIBA2_FIELDTYPES_H_

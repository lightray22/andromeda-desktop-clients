#ifndef LIBA2_SCALARTYPE_H_
#define LIBA2_SCALARTYPE_H_

#include <cstddef>
#include <string>

#include "BaseField.hpp"

namespace Andromeda {
namespace Database {
class BaseObject;

namespace FieldTypes {

/**
 * A field holding a possibly-null scalar type
 * @tparam T supported scalar type (std::string, int, int64_t, double)
 */
template<typename T>
class NullScalarType : public BaseField
{
public:
    /** Construct with a null default value */
    using BaseField::BaseField;

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
    inline bool operator==(std::nullptr_t) const { return is_null(); }
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

/**
 * A field holding a non-null scalar type
 * @tparam T supported scalar type (std::string, int, int64_t, double)
 */
template<typename T>
class ScalarType : public BaseField
{
public:
    /** Construct uninitialized */
    using BaseField::BaseField;

    /** Construct with a default value and set dirty */
    ScalarType(const char* name, BaseObject& parent, const T& defaultt) :
        BaseField(name, parent, 1), 
        mTempInitd(true), mTempValue(defaultt),
        mRealInitd(true), mRealValue(defaultt) { }

    /** @throws DBValueNullException if the given DB value is null */
    void InitDBValue(const MixedValue& value) override
    {
        if (value.is_null()) throw DBValueNullException(mName);

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
        else return MixedValue(mRealValue);
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

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

#endif // LIBA2_SCALARTYPE_H_

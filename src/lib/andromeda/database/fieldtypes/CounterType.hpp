#ifndef LIBA2_COUNTERTYPE_H_
#define LIBA2_COUNTERTYPE_H_

#include "BaseField.hpp"

namespace Andromeda {
namespace Database {
namespace FieldTypes {

/** A field holding a incrementable counter */
class CounterType : public BaseField
{
public:
    /** Construct with a 0 default value */
    using BaseField::BaseField;

    [[nodiscard]] bool UseDBIncrement() const override { return true; }

    void InitDBValue(const MixedValue& value) override
    {
        mDelta = 0;
        value.get_to(mValue);
    }

    [[nodiscard]] MixedValue GetDBValue() const override { return MixedValue(mDelta); }

    /** Returns the field's value */
    [[nodiscard]] int GetValue() const { return mValue; }

    /** Returns the field's value */
    inline explicit operator int() const { return GetValue(); }

    /** Return true if equal to the given value */
    inline bool operator==(int value) const { return GetValue() == value; }

    /** Return true if not equal to the given value */
    inline bool operator!=(int rhs) const { return !(*this==rhs); }

    /**
     * Increments the counter by the given value
     * @return true if the value is modified
     */
    bool DeltaValue(int delta = 1)
    {
        if (!delta) return false;

        notifyModified();
        mValue += delta;
        mDelta += delta;
        return true;
    }

    /** Increments the counter by the given value */
    inline CounterType& operator+=(int delta) { DeltaValue(delta); return *this; }
    /** Decrements the counter by the given value */
    inline CounterType& operator-=(int delta) { DeltaValue(-delta); return *this; }

protected:
    int mValue { 0 };
};

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

#endif // LIBA2_COUNTERTYPE_H_

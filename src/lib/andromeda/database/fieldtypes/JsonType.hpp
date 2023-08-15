#ifndef LIBA2_JSONTYPE_H_
#define LIBA2_JSONTYPE_H_

#include <cstddef>
#include <memory>
#include "nlohmann/json_fwd.hpp"

#include "BaseField.hpp"

namespace Andromeda {
namespace Database {
class BaseObject;

namespace FieldTypes {

/** A field holding a possibly-null JSON value */
class JsonType : public BaseField
{
public:
    /** Construct with a null default value */
    using BaseField::BaseField;

    /** Exception indicating a JSON decoding failed */
    class JsonDecodeException : public DatabaseException { public:
        explicit JsonDecodeException(const std::string& errTxt) :
            DatabaseException("JSON decode error: "+errTxt) {}; };

    /** Construct with a non-null default value and set dirty */
    JsonType(const char* name, BaseObject& parent, const nlohmann::json& defaultt);

    /** @throws JsonDecodeException if the string is bad */
    void InitDBValue(const MixedValue& value) override;

    [[nodiscard]] MixedValue GetDBValue() const override;

    /** Returns true if the value is NULL */
    [[nodiscard]] inline bool is_null() const { return mJsonPtr == nullptr; }

    /** Returns a pointer to the value or nullptr if NULL */
    [[nodiscard]] const nlohmann::json* TryGetJson() const { return mJsonPtr.get(); }

    /** Returns a pointer to the value or nullptr if NULL */
    inline explicit operator const nlohmann::json*() const { return TryGetJson(); }
    /** Returns a reference to the value (assumes not NULL) */
    inline const nlohmann::json& operator*() const { return *TryGetJson(); }
    /** Return true if NULL */
    inline bool operator==(std::nullptr_t) const { return is_null(); }
    /** Return true if NOT NULL */
    inline bool operator!=(std::nullptr_t) const { return !(*this==nullptr); }

    /** Sets the field to the given value */
    void SetJson(const nlohmann::json& value);
    /** Sets the field to NULL */
    void SetJson(std::nullptr_t);

    /** Sets the field to the given value */
    inline JsonType& operator=(const nlohmann::json& value) noexcept { SetJson(value); return *this; }
    /** Sets the field to NULL */
    inline JsonType& operator=(std::nullptr_t) noexcept { SetJson(nullptr); return *this; }

protected:
    std::unique_ptr<nlohmann::json> mJsonPtr;
};

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

#endif // LIBA2_COUNTERTYPE_H_

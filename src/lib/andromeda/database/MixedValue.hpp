#ifndef LIBA2_MIXEDVALUE_H_
#define LIBA2_MIXEDVALUE_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <variant>

struct sqlite3_stmt;
struct sqlite3_value;

namespace Andromeda {
namespace Database {

/**
 * Holds a mixed type value either as a sqlite3_value or a std::variant
 * Can be converted to the underlying type and bind appropriately to an sqlite3_stmt 
 * Sqlite values can be casted as any type, but if holding a variant, the type requested must match
 */
class MixedValue
{
public:
    /** Construct an input from any allowed variant */
    template<typename T>
    explicit MixedValue(const T& value) noexcept : mVariant(value) { }

    // allow std::string& rather than std::string* (variant can't hold a &)
    explicit MixedValue(const std::string& value) noexcept : mVariant(&value) { }

    /** Initialize (copy) from an sqlite3_value */
    explicit MixedValue(const sqlite3_value& value) noexcept;

    virtual ~MixedValue(); // delete sqlite3_value

    MixedValue(const MixedValue& src) noexcept; // copy
    MixedValue(MixedValue&& old) noexcept; // move

    MixedValue& operator=(const MixedValue&) = delete;
    MixedValue& operator=(MixedValue&&) = delete;

    /** 
     * Bind the value to the statement with the appropriate sqlite function 
     * @return the sqlite result code
     */
    int bind(sqlite3_stmt& stmt, int idx) const;

    /** Returns true if the value is SQL NULL */
    [[nodiscard]] bool is_null() const;

    /** 
     * Stores the value in the desired out variable 
     * @throws std::bad_variant_access if holding a variant of a different type
     */
    void get_to(std::string& out) const;
    void get_to(const char*& out) const;
    void get_to(int& out) const;
    void get_to(int64_t& out) const;
    void get_to(double& out) const;

    /** 
     * Returns the value as the desired type 
     * @throws std::bad_variant_access if holding a variant of a different type
     */
    template <typename T>
    [[nodiscard]] inline T get() const
    {
        T out; get_to(out); return out;
    }

    /** 
     * Compare to any type by first casting to it 
     * @throws std::bad_variant_access if holding a variant of a different type
     */
    template<typename T>
    inline bool operator==(const T& cmp) const
    {
        return get<T>() == cmp;
    }

    /** Can't cast to std::nullptr_t so use is_null() */
    inline bool operator==(std::nullptr_t) const
    {
        return is_null();
    }

    /** 
     * Allow comparing correctly to const char* using strcmp 
     * @throws std::bad_variant_access if holding a variant of a different type
     */
    inline bool operator==(const char* str) const
    {
        return !std::strcmp(get<const char*>(), str);
    }

private:
    /** The mixed-type value from sqlite */
    sqlite3_value* mSqlValue { nullptr };

    /** The manually-set value of any allowable type */
    std::variant<std::nullptr_t, const std::string*, const char*, int, int64_t, double> mVariant;
};

using MixedParamsBase = std::unordered_map<std::string, MixedValue>;

/**
 * A pair of a param name and mixed type value
 * This class allows nice mixed-type syntax, e.g. {{":d0",5},{":d1","test"}}
 */
class MixedValuePair : public MixedParamsBase::value_type
{
public:
    /** Construct a pair from a name and any allowed value type */
    template<typename T>
    MixedValuePair(const char* name, const T& value) noexcept :
        MixedParamsBase::value_type(name, value) { } // converts T value to MixedValue
};

/** Map of param name to mixed type inputs */
class MixedParams : public MixedParamsBase
{
public:
    MixedParams() = default;
    /** Initialize the map from a list of MixedValuePairs */
    MixedParams(const std::initializer_list<MixedValuePair>& list) noexcept;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_MIXEDVALUE_H_

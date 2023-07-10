#ifndef LIBA2_MIXEDINPUT_H_
#define LIBA2_MIXEDINPUT_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

struct sqlite3_stmt;

namespace Andromeda {
namespace Database {

/** Holds a mixed type value and can bind it appropriately to an sqlite3_stmt */
class MixedInput
{
public:
    /** Construct an input from any allowed value type */
    template<typename T>
    explicit MixedInput(const T& value) noexcept : mValue(value) { }

    // allow std::string& rather than std::string* (variant can't hold a &)
    explicit MixedInput(const std::string& value) noexcept : mValue(&value) { }

    /** Bind the value to the statement with the appropriate sqlite function */
    int bind(sqlite3_stmt& stmt, int idx) const;

    bool operator==(const MixedInput& cmp) const
    {
        // use std::variant operator==
        return mValue == cmp.mValue; 
    }

private:
    /** The value of any allowable type */
    std::variant<std::nullptr_t, const std::string*, const char*, int, int64_t, double> mValue;
};

using MixedParamsBase = std::unordered_map<std::string, MixedInput>;

/** A pair of a param name and mixed type value */
class MixedInputPair : public MixedParamsBase::value_type
{
public:
    /** Construct a pair from a name and any allowed value type */
    template<typename T>
    MixedInputPair(const char* name, const T& value) noexcept :
        MixedParamsBase::value_type(name, value) { } // converts T value to MixedInput
};

/** Map of param name to mixed type inputs */
class MixedParams : public MixedParamsBase
{
public:
    MixedParams() = default;
    /**
     * Initialize the map from a list of MixedInputPairs
     * These classes allow nice mixed-type syntax, e.g. {{":d0",5},{":d1","test"}}
     */
    MixedParams(const std::initializer_list<MixedInputPair>& list) noexcept;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_MIXEDINPUT_H_

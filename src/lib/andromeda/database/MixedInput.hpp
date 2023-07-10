#ifndef LIBA2_MIXEDINPUT_H_
#define LIBA2_MIXEDINPUT_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <variant>

struct sqlite3_stmt;

namespace Andromeda {
namespace Database {

/** Holds a mixed type value and can bind it appropriately to an sqlite3_stmt */
struct MixedInput
{
    /** Construct an input from any allowed value type */
    template<typename T>
    explicit MixedInput(const T& value) noexcept : mValue(value) { }

    // allow std::string& rather than std::string*
    explicit MixedInput(const std::string& value) noexcept : mValue(&value) { }

    /** Bind the value to the statement with the appropriate sqlite function */
    int bind(sqlite3_stmt& stmt, int idx) const;

    /** The value of any allowable type */
    std::variant<std::nullptr_t, const std::string*, const char*, int, int64_t, double> mValue;
};

/** A pair of a param name and mixed type value */
struct MixedInputPair
{
    /** Construct a pair from a name and any allowed value type */
    template<typename T>
    MixedInputPair(const char* name, const T& value) noexcept :
        mName(name), mInput(value) { } // convert value to MixedInput
    
    const char* mName;
    MixedInput mInput;
};

/** Map of param name to mixed type inputs */
class MixedParams : public std::map<std::string,MixedInput>
{
public:
    /**
     * Initialize the map from a list of MixedInputPairs
     * These classes allow nice mixed-type syntax, e.g. {{":d0",5},{":d1","test"}}
     */
    MixedParams(const std::initializer_list<MixedInputPair>& list) noexcept;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_MIXEDINPUT_H_

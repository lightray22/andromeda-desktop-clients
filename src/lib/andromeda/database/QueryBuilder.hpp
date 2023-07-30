#ifndef LIBA2_QUERYBUILDER_H_
#define LIBA2_QUERYBUILDER_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <list>
#include <string>

#include "MixedValue.hpp"
#include "andromeda/StringUtil.hpp"

namespace Andromeda {
namespace Database {

/** 
 * Minimalistic class for building prepared post-FROM SQL query strings
 * Mostly ported from the server's PHP implementation
 */
class QueryBuilder
{
public:

    /** Return the query param replacement map */
    [[nodiscard]] inline const MixedParams& GetParams() const { return mParams; }

    /** Returns the compiled query as a string */
    [[nodiscard]] std::string GetText() const;

    /** Returns a string asserting the given column is null */
    [[nodiscard]] std::string IsNull(const char* key) const
    {
        return std::string(key)+" IS NULL";
    }

    /** Returns the given string with SQL wildcard characters escaped */
    [[nodiscard]] static std::string EscapeWildcards(const std::string& q);

    /** 
     * Returns a string comparing the given column to a string value using LIKE 
     * NOTE if using hasMatch, MAKE SURE input is ESCAPED with EscapeWildcards!
     * @param hasMatch if true, the string manages its own SQL wildcard characters else use %val%
     */
    template<typename T> // const char* or std::string
    [[nodiscard]] std::string Like(const char* key, const T& val, bool hasMatch = false)
    {
        const std::string qval { hasMatch ? val : "%"+EscapeWildcards(val)+"%" };
        return std::string(key)+" LIKE "+AddParam(qval)+" ESCAPE '\\'";
    }

    /** Returns a query string asserting the given column is less than the given value */
    template<typename T>
    [[nodiscard]] std::string LessThan(const char* key, const T& val)
    {
        return std::string(key)+" < "+AddParam(val);
    }

    /** Returns a query string asserting the given column is less than or equal to the given value */
    template<typename T>
    [[nodiscard]] std::string LessThanEquals(const char* key, const T& val)
    {
        return std::string(key)+" <= "+AddParam(val);
    }

    /** Returns a query string asserting the given column is greater than the given value */
    template<typename T>
    [[nodiscard]] std::string GreaterThan(const char* key, const T& val)
    {
        return std::string(key)+" > "+AddParam(val);
    }

    /** Returns a query string asserting the given column is greater than or equal to the given value */
    template<typename T>
    [[nodiscard]] std::string GreaterThanEquals(const char* key, const T& val)
    {
        return std::string(key)+" >= "+AddParam(val);
    }

    /** Returns a query string asserting the given column is "true" (greater than zero) */
    [[nodiscard]] std::string IsTrue(const char* key)
    {
        return GreaterThan(key,0);
    }

    /** Returns a query string asserting the given column is equal to the given value */
    template<typename T>
    [[nodiscard]] std::string Equals(const char* key, const T& val)
    {
        return std::string(key)+" = "+AddParam(val);
    }

    /* Returns a query string asserting the given column is equal to the given value (nullptr) */
    [[nodiscard]] std::string Equals(const char* key, std::nullptr_t val) // NOLINT(*-make-member-function-const) // https://github.com/llvm/llvm-project/issues/63787
    {
        return IsNull(key);
    }

    /** Returns a query string asserting the given column is not equal to the given value */
    template<typename T>
    [[nodiscard]] std::string NotEquals(const char* key, const T& val)
    {
        return std::string(key)+" <> "+AddParam(val);
    }

    /* Returns a query string asserting the given column is not equal to the given value (nullptr) */
    [[nodiscard]] std::string NotEquals(const char* key, std::nullptr_t val) // NOLINT(*-make-member-function-const) // https://github.com/llvm/llvm-project/issues/63787
    {
        return Not(IsNull(key));
    }

    /**
     * Syntactic sugar function to check many OR conditions at once
     * @param vals list of possible values for the column
     */
    [[nodiscard]] std::string ManyEqualsOr(const char* key, const std::initializer_list<const char*>& vals)
    {
        std::vector<std::string> parts(vals.size());
        std::transform(std::begin(vals), std::end(vals), std::begin(parts), 
            [&](const char* val)->std::string { return Equals(key, val); });
        return "("+StringUtil::implode(" OR ",parts)+")";
    }
    
    // TODO could add ManyEqualsAnd that takes in MixedParams?

    /** Returns a query string that inverts the logic of the given query */
    [[nodiscard]] std::string Not(const std::string& arg) const
    {
        return "(NOT "+arg+")";
    }

    /** Returns a query string that combines the given arguments using OR */
    template<typename... Ts>
    [[nodiscard]] std::string Or(Ts&&... args) const
    {
        // convert parameter pack to array of strings
        const std::array<std::string, sizeof...(Ts)> strs {{ std::forward<Ts>(args) ... }};
        return "("+StringUtil::implode(" OR ",strs)+")";
    }

    /** Returns a query string that combines the given arguments using AND */
    template<typename... Ts>
    [[nodiscard]] std::string And(Ts&&... args) const
    {
        // convert parameter pack to array of strings
        const std::array<std::string, sizeof...(Ts)> strs {{ std::forward<Ts>(args) ... }};
        return "("+StringUtil::implode(" AND ",strs)+")";
    }

    /**
      * Assigns/adds a WHERE clause to the query
      * if null, resets - if called > once, uses AND 
     */
    inline QueryBuilder& Where(const std::string& where)
    {
        if (!where.empty() && !mWhere.empty())
            mWhere = And(mWhere, where);
        else mWhere = where;
        return *this;
    }

    /**
      * Assigns/adds a WHERE clause to the query
      * if null, resets - if called > once, uses AND 
     */
    inline QueryBuilder& Where(std::nullptr_t)
    {
        mWhere.clear();
        return *this;
    }

    /** Returns the current WHERE string (or empty if none) */
    [[nodiscard]] inline const std::string& GetWhere() const { return mWhere; }

    /** Assigns an ORDER BY clause to the query, optionally descending, nullptr to reset */
    inline QueryBuilder& OrderBy(const char* orderby, bool desc = false)
    {
        mOrderBy = orderby;
        mOrderDesc = desc;
        return *this;
    }

    /** Assigns an ORDER BY clause to the query, optionally descending, nullptr to reset */
    inline QueryBuilder& OrderBy(std::nullptr_t)
    {
        mOrderBy.clear();
        mOrderDesc = false;
        return *this;
    }

    /** Returns the current ORDER BY key (or empty if none) */
    [[nodiscard]] inline const std::string& GetOrderBy() const { return mOrderBy; }

    /** Returns true if the order is descending */
    [[nodiscard]] inline bool GetOrderDesc() const { return mOrderDesc; }

    /** Assigns a LIMIT clause to the query, null to reset */
    inline QueryBuilder& Limit(size_t limit)
    {
        mLimit = limit;
        mLimitNull = false;
        return *this;
    }

    /** Assigns a LIMIT clause to the query, null to reset */
    inline QueryBuilder& Limit(std::nullptr_t)
    {
        mLimitNull = true;
        return *this;
    }

    /** Assigns an OFFSET clause to the query (use with LIMIT), null to reset */
    inline QueryBuilder& Offset(size_t offset)
    {
        mOffset = offset;
        mOffsetNull = false;
        return *this;
    }

    /** Assigns an OFFSET clause to the query (use with LIMIT), null to reset */
    inline QueryBuilder& Offset(std::nullptr_t)
    {
        mOffsetNull = true;
        return *this;
    }

    /** Returns a pointer to the current LIMIT or nullptr if none */
    [[nodiscard]] inline const size_t* GetLimit() const
    {
        if (mLimitNull) return nullptr;
        else return &mLimit;
    }

    /** Returns a pointer to the current OFFSET or nullptr if none */
    [[nodiscard]] inline const size_t* GetOffset() const
    {
        if (mOffsetNull) return nullptr;
        else return &mOffset;
    }

private:

    /**
     * Adds the given value to the internal param array
     * @return the placeholder string to go in the query
     */
    template<typename T>
    [[nodiscard]] std::string AddParam(const T& val)
    {
        std::string idx { ":d"+std::to_string(mParamIdx++) };
        mParams.emplace(idx, val); 
        return idx; // non-const for move
    }

    /** Next parameter index to use */
    size_t mParamIdx { 0 };

    /** The current WHERE query string */
    std::string mWhere;
    /** The current ORDER BY string */
    std::string mOrderBy;
    /** true if ORDER BY is DESC */
    bool mOrderDesc { false };
    /** true if no LIMIT is set */
    bool mLimitNull { true };
    /** The current LIMIT value */
    size_t mLimit { 0 };
    /** true if no OFFSET is set */
    bool mOffsetNull { true };
    /** The current OFFSET value */
    size_t mOffset { 0 };

    /** variables to be substituted in the query */
    MixedParams mParams;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_QUERYBUILDER_H_

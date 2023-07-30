#ifndef LIBA2_TABLEBUILDER_H_
#define LIBA2_TABLEBUILDER_H_

#include <algorithm>
#include <array>
#include <list>
#include <string>
#include <vector>

#include "andromeda/StringUtil.hpp"

namespace Andromeda {
namespace Database {

/** Minimalistic class for building SQLite table creation strings */
class TableBuilder
{
public:

    /** Creates a TableBuilder for a BaseObject class */
    template<class T>
    static TableBuilder For()
    {
        return TableBuilder(T::GetClassNameS());
    }

    /** Returns the compiled queries as a list of strings */
    [[nodiscard]] std::list<std::string> GetQueries() const;

    /**
     * Adds a column to the table
     * @param name name of the column
     * @param type SQL datatype of the column
     * @param null if true, allow+default NULL
     */
    TableBuilder& AddColumn(const std::string& name, const std::string& type, bool null) 
    // `id` integer NOT NULL
    {
        mColumns.emplace_back("`"+name+"` "+type
            +" "+(null?"DEFAULT":"NOT")+" NULL"); return *this;
    }

    /** Sets the primary key for the given to the given column */
    TableBuilder& SetPrimary(const std::string& name) 
    // PRIMARY KEY (`id`)
    {
        mPrimary = "PRIMARY KEY (`"+name+"`)"; return *this;
    }

    /** Creates a UNIQUE constraint for the given set of columns */
    template<typename... Ts>
    TableBuilder& AddUnique(Ts&&... args) 
    // UNIQUE (`field1`,`field2`)
    {
        // convert parameter pack to array of strings
        const std::array<std::string, sizeof...(Ts)> strs {{ std::forward<Ts>(args) ... }};

        mUniques.emplace_back("UNIQUE "+FormatFields(strs)); return *this;
    }

    /** Adds a quick-lookup INDEX for the given set of columns */
    template<typename... Ts>
    TableBuilder& AddIndex(Ts&&... args) 
    // CREATE INDEX "idx_a2obj_easyobject_field1_field2" ON "a2obj_easyobject" (`field1`,`field2`)
    {
        // convert parameter pack to array of strings
        const std::array<std::string, sizeof...(Ts)> strs {{ std::forward<Ts>(args) ... }};

        const std::string table { GetTableName(mClassName) };

        mIndexes.emplace_back("CREATE INDEX \"idx_"+table+"_"+StringUtil::implode("_",strs)
            +"\" ON \""+table+"\" "+FormatFields(strs)); return *this;
    }

    enum class OnDelete { RESTRICT, SET_NULL, CASCADE };
    static constexpr const char* OnDeleteStrs[] { "RESTRICT", "SET NULL", "CASCADE" }; // NOLINT(*-avoid-c-arrays)

    /**
     * Adds a foreign key constraint to the table
     * @tparam Ref other class being referenced
     * @param ourKey the key in our table referencing refKey
     * @param refKey the key in the other table being referenced
     * @param del action to take when Ref is deleted
     */
    template<typename Ref>
    TableBuilder& AddConstraint(const char* ourKey, const char* refKey, OnDelete del = OnDelete::RESTRICT) 
    // CONSTRAINT `a2obj_easyobject_ibfk_1` FOREIGN KEY (`key`) REFERENCES `a2obj_easyobject2` (`key2`) ON DELETE SET NULL
    {
        const std::string table1 { GetTableName(mClassName) };
        const std::string table2 { GetTableName(Ref::GetClassNameS()) };

        mConstraints.emplace_back("CONSTRAINT `"+table1+"_ibfk_"+std::to_string(mConstraintIdx++)+"`"
            +" FOREIGN KEY (`"+ourKey+"`) REFERENCES `"+table2+"` (`"+refKey+"`)"
            +" ON DELETE "+OnDeleteStrs[static_cast<size_t>(del)]); return *this;
    }

private:

    /** Create a TableBuilder with the given class name */
    explicit TableBuilder(const std::string& className):
        mClassName(className) { }

    /** Returns the database table name for a class name */
    std::string GetTableName(const std::string& className);

    /** Return the given container of strings enclosed in `` and joined with , */
    template<typename T>
    std::string FormatFields(const T& strs) // `field1`,`field2`
    {
        std::vector<std::string> fields(strs.size());
        std::transform(strs.begin(), strs.end(), fields.begin(), 
            [](const std::string& str){ return "`"+str+"`"; });

        return "("+StringUtil::implode(",",fields)+")";
    }

    /** The BaseObject class name */
    const std::string mClassName;
    /** List of columns that make up the table */
    std::list<std::string> mColumns;
    /** Primary key for the table */
    std::string mPrimary;
    /** List of UNIQUE key clauses */
    std::list<std::string> mUniques;
    /** List of CREATE INDEX queries */
    std::list<std::string> mIndexes;
    /** Next constraint index to use */
    size_t mConstraintIdx { 1 };
    /** List of CONSTRAINT clauses */
    std::list<std::string> mConstraints;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_TABLEBUILDER_H_

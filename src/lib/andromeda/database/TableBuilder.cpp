
#include <iterator>

#include "ObjectDatabase.hpp"
#include "TableBuilder.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
std::string TableBuilder::GetTableName(const std::string& className)
{
    return ObjectDatabase::GetClassTableName(className);
}

/*****************************************************/
std::list<std::string> TableBuilder::GetQueries() const
{
    std::list<std::string> props = mColumns; // copy
    if (!mPrimary.empty()) props.emplace_back(mPrimary);
    std::copy(mUniques.cbegin(), mUniques.cend(), std::back_inserter(props));
    std::copy(mConstraints.cbegin(), mConstraints.cend(), std::back_inserter(props));

    std::list<std::string> queries;
    const std::string table { ObjectDatabase::GetClassTableName(mClassName) };
    queries.emplace_front("CREATE TABLE `"+table+"` ("+Utilities::implode(", ",props)+")");
    std::copy(mIndexes.cbegin(), mIndexes.cend(), std::back_inserter(queries)); // each CREATE INDEX is a query
    return queries;
}

} // namespace Database
} // namespace Andromeda

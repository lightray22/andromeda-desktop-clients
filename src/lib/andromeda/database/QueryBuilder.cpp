
#include <utility>

#include "QueryBuilder.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
std::string QueryBuilder::EscapeWildcards(const std::string& q)
{
    return Utilities::escapeAll(q,{'_','%'},'\\');
}

/*****************************************************/
std::string QueryBuilder::GetText() const
{
    std::string query;

    if (!mWhere.empty())
        query += " WHERE "+mWhere;

    if (!mOrderBy.empty())
    {
        query += " ORDER BY "+mOrderBy;
        if (mOrderDesc) query += " DESC"; // default is ASC
    }

    if (!mLimitNull)
        query += " LIMIT "+std::to_string(mLimit);

    if (!mOffsetNull)
        query += " OFFSET "+std::to_string(mOffset);

    Utilities::trim_void(query); return query;
}

} // namespace Database
} // namespace Andromeda

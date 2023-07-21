#ifndef LIBA2_TESTOBJECTS_H_
#define LIBA2_TESTOBJECTS_H_

#include <string>

#include "catch2/catch_test_macros.hpp"
#include "catch2/trompeloeil.hpp"

#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/FieldTypes.hpp"
#include "andromeda/database/MixedValue.hpp"
#include "andromeda/database/SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {

class MockSqliteDatabase : public SqliteDatabase { public:
    MockSqliteDatabase() = default;
    MAKE_MOCK2(query, size_t(const std::string&, const MixedParams&), override);
    MAKE_MOCK3(query, size_t(const std::string&, const MixedParams&, RowList&), override);
};

class EasyObject : public BaseObject
{
public:
    BASEOBJECT_NAME(EasyObject, "Andromeda\\Database\\EasyObject")

    EasyObject(ObjectDatabase& database, const MixedParams& data) : 
        BaseObject(database, data)
    {

    }

};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_BASEOBJECT_H_

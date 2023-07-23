#ifndef LIBA2_TESTOBJECTS_H_
#define LIBA2_TESTOBJECTS_H_

#include <string>

#include "catch2/catch_test_macros.hpp"
#include "catch2/trompeloeil.hpp"

#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/FieldTypes.hpp"
#include "andromeda/database/MixedValue.hpp"
#include "andromeda/database/ObjectDatabase.hpp"
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

    BASEOBJECT_CONSTRUCT(EasyObject)
        mMyStr("mystr",*this),
        mMyInt("myint",*this)
    {
        BASEOBJECT_REGISTER(&mMyStr, &mMyInt)
    }

    static EasyObject& Create(ObjectDatabase& db, int myInt)
    {
        EasyObject& obj { db.CreateObject<EasyObject>() };
        obj.mMyInt.SetValue(myInt);
        return obj;
    }

    bool setMyStr(const std::string& myStr)
    {
        return mMyStr.SetValue(myStr);
    }

    const std::string* getMyStr() const
    {
        return mMyStr.TryGetValue();
    }

private:
    FieldTypes::NullScalarType<std::string> mMyStr;
    FieldTypes::ScalarType<int> mMyInt;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_BASEOBJECT_H_

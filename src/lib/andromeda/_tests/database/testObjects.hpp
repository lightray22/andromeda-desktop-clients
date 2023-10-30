#ifndef LIBA2_TESTOBJECTS_H_
#define LIBA2_TESTOBJECTS_H_

#include <functional>
#include <string>

#include "catch2/catch_test_macros.hpp"
#include "catch2/trompeloeil.hpp"

#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/MixedValue.hpp"
#include "andromeda/database/ObjectDatabase.hpp"
#include "andromeda/database/SqliteDatabase.hpp"
#include "andromeda/database/fieldtypes/CounterType.hpp"
#include "andromeda/database/fieldtypes/ScalarType.hpp"

namespace Andromeda {
namespace Database {

class MockSqliteDatabase : public SqliteDatabase { public:
    MockSqliteDatabase() = default;
    MAKE_MOCK3(query, size_t(const std::string&, const MixedParams&, RowList&), override);
};

class MockObjectDatabase : public ObjectDatabase { public:
    using ObjectDatabase::ObjectDatabase;
    MAKE_MOCK1(notifyModified, void(BaseObject&), override);
};

class EasyObject : public BaseObject
{
public:
    BASEOBJECT_NAME(EasyObject, "Andromeda\\Database\\EasyObject")

    EasyObject(ObjectDatabase& database, const MixedParams& data, bool created=false):
        BaseObject(database),
        mMyStr("mystr",*this),
        mMyInt("myint",*this),
        mCounter("myctr",*this)
    {
        RegisterFields({&mMyStr, &mMyInt, &mCounter});
        InitializeFields(data, created);
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

    void deltaCounter(int delta)
    {
        mCounter.DeltaValue(delta);
    }

    void onDelete(const std::function<void()>& func)
    {
        mOnDelete = func;
    }

    void NotifyPostDeleted() override
    {
        if (mOnDelete) mOnDelete();
    }

private:

    FieldTypes::NullScalarType<std::string> mMyStr;
    FieldTypes::ScalarType<int> mMyInt;
    FieldTypes::CounterType mCounter;

    std::function<void()> mOnDelete;
};

class EasyObject2 : public BaseObject
{
public:
    BASEOBJECT_NAME(EasyObject2, "Andromeda\\Database\\EasyObject2")

    EasyObject2(ObjectDatabase& database, const MixedParams& data, bool created=false):
        BaseObject(database),
        mMyInt("myint",*this)
    {
        RegisterFields({&mMyInt});
        InitializeFields(data, created);
    }

private:
    FieldTypes::ScalarType<int> mMyInt;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_BASEOBJECT_H_

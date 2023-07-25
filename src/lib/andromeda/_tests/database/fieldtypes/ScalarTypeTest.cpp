
#include "catch2/catch_test_macros.hpp"

#include "../testObjects.hpp"
#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/ObjectDatabase.hpp"
#include "andromeda/database/fieldtypes/ScalarType.hpp"

namespace Andromeda {
namespace Database {
namespace FieldTypes {

#define GET_MOCK_OBJECTS() \
    MockSqliteDatabase sqldb; \
    ObjectDatabase objdb(sqldb); \
    EasyObject parent(objdb,{});

/*****************************************************/
TEST_CASE("Basic", "[ScalarType]")
{
    GET_MOCK_OBJECTS();

    ScalarType<std::string> field("myfield",parent);

    REQUIRE(std::string(field.GetName()) == "myfield");
    REQUIRE(field.GetDelta() == 0);
    REQUIRE(field.isModified() == false);
    
    REQUIRE(field.SetValue("test") == true);
    REQUIRE(field.SetValue("test") == false);
    REQUIRE(field.GetDelta() == 1);
    REQUIRE(field.isModified() == true);
}

/*****************************************************/
TEST_CASE("Operators", "[ScalarType]")
{
    GET_MOCK_OBJECTS();

    ScalarType<std::string> field1("myfield",parent);
    field1 = "test2";
    REQUIRE(static_cast<const std::string>(field1) == "test2");
    REQUIRE(field1 == "test2"); // operator==
    REQUIRE(field1 != "test3"); // operator!=

    NullScalarType<std::string> field2("myfield",parent);
    field2 = nullptr;
    REQUIRE(field2 == nullptr);
    REQUIRE(static_cast<const std::string*>(field2) == nullptr);
    field2 = "test3";
    REQUIRE(field2 != nullptr);
    REQUIRE(*field2 == "test3");
    REQUIRE(*static_cast<const std::string*>(field2) == "test3");
}

/*****************************************************/
TEST_CASE("SetUnmodified", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    ScalarType<std::string> field("myfield",parent);

    field.SetValue("test");
    REQUIRE(field.GetDelta() == 1);

    field.SetUnmodified();
    REQUIRE(field.GetDelta() == 0);
}

/*****************************************************/
TEST_CASE("Uninitialized", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    const ScalarType<std::string> field("myfield",parent);
    REQUIRE_THROWS_AS(field.GetValue(), BaseField::UninitializedException);
}

/*****************************************************/
TEST_CASE("NullStringValue", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    NullScalarType<std::string> field("myfield",parent);

    REQUIRE(field.SetValue(nullptr) == false);
    REQUIRE(field.is_null() == true);
    REQUIRE(field.TryGetValue() == nullptr);

    REQUIRE(field.SetValue("test") == true);
    REQUIRE(field.is_null() == false);
    REQUIRE(*field.TryGetValue() == "test");

    REQUIRE(field.SetValue("test") == false);
    REQUIRE(*field.TryGetValue() == "test");

    REQUIRE(field.SetValue("test2") == true);
    REQUIRE(*field.TryGetValue() == "test2");

    REQUIRE(field.GetDelta() == 2);

    REQUIRE(field.SetValue(nullptr) == true);
    REQUIRE(field.is_null() == true);
}

/*****************************************************/
TEST_CASE("NullStringTempValue", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    NullScalarType<std::string> field("myfield",parent);

    REQUIRE(field.SetValue("test",true) == false);
    REQUIRE(*field.TryGetValue() == "test");
    REQUIRE(field.TryGetValue(false) == nullptr);
    REQUIRE(field.GetDBValue() == nullptr);
    REQUIRE(field.GetDelta() == 0);
    
    REQUIRE(field.SetValue("test") == true);
    REQUIRE(field.GetDelta() == 1);
}

/*****************************************************/
TEST_CASE("NullStringDBValue", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    NullScalarType<std::string> field("myfield",parent);

    field.InitDBValue(MixedValue(nullptr)); REQUIRE(field.TryGetValue() == nullptr); REQUIRE(field.GetDBValue() == nullptr);
    field.InitDBValue(MixedValue("")); REQUIRE(field.TryGetValue()->empty()); REQUIRE(field.TryGetValue()->empty());
    field.InitDBValue(MixedValue("5")); REQUIRE(*field.TryGetValue() == "5"); REQUIRE(field.GetDBValue() == "5");
}

/*****************************************************/
TEST_CASE("NullStringDefault", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    {
        const NullScalarType<std::string> field("myfield",parent);
        REQUIRE(field.TryGetValue() == nullptr);
    }

    {
        const NullScalarType<std::string> field("myfield",parent,"a");
        REQUIRE(*field.TryGetValue() == "a");
        REQUIRE(field.GetDelta() == 1);
    }
}

/*****************************************************/
TEST_CASE("StringValue", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    ScalarType<std::string> field("myfield",parent);

    REQUIRE(field.SetValue("test") == true);
    REQUIRE(field.GetValue() == "test");

    REQUIRE(field.SetValue("test") == false);
    REQUIRE(field.GetValue() == "test");

    REQUIRE(field.SetValue("test2") == true);
    REQUIRE(field.GetValue() == "test2");

    REQUIRE(field.GetDelta() == 2);
}

/*****************************************************/
TEST_CASE("StringTempValue", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    ScalarType<std::string> field("myfield",parent);
    field.InitDBValue(MixedValue("init"));

    REQUIRE(field.SetValue("test",true) == false);
    REQUIRE(field.GetValue() == "test");
    REQUIRE(field.GetValue(false) == "init");
    REQUIRE(field.GetDBValue() == "init");
    REQUIRE(field.GetDelta() == 0);
    
    REQUIRE(field.SetValue("test") == true);
    REQUIRE(field.GetDelta() == 1);
}

/*****************************************************/
TEST_CASE("StringDBValue", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    ScalarType<std::string> field("myfield",parent);

    field.SetValue("tmp",true);
    REQUIRE(field.isInitialized() == true);
    REQUIRE(field.isInitialized(false) == false);

    field.InitDBValue(MixedValue("")); REQUIRE(field.GetValue().empty()); REQUIRE(field.GetValue().empty());
    field.InitDBValue(MixedValue("5")); REQUIRE(field.GetValue() == "5"); REQUIRE(field.GetDBValue() == "5");

    REQUIRE(field.isInitialized() == true);
}

/*****************************************************/
TEST_CASE("StringDefault", "[ScalarType]")
{
    GET_MOCK_OBJECTS();
    
    const ScalarType<std::string> field("myfield",parent,"a");
    REQUIRE(field.GetValue() == "a");
    REQUIRE(field.GetDelta() == 1);
}

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

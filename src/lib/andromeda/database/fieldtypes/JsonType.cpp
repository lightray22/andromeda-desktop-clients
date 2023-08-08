
#include "nlohmann/json.hpp"
#include "JsonType.hpp"

namespace Andromeda {
namespace Database {
namespace FieldTypes {

/*****************************************************/
JsonType::JsonType(const char* name, BaseObject& parent, const nlohmann::json& defaultt) :
    BaseField(name, parent, 1) { SetJson(defaultt); }

/*****************************************************/
void JsonType::InitDBValue(const MixedValue& value)
{
    mDelta = 0;
    if (value == nullptr) mJsonPtr = nullptr;
    else try
    {
        mJsonPtr = std::make_unique<nlohmann::json>(
            nlohmann::json::parse(value.get<std::string>()));
    }
    catch (const nlohmann::json::exception& ex) {
        throw JsonDecodeException(ex.what());
    }
}

/*****************************************************/
MixedValue JsonType::GetDBValue() const
{
    if (mJsonPtr == nullptr) return MixedValue(nullptr);
    return MixedValue(mJsonPtr->dump()); // string
}

/*****************************************************/
void JsonType::SetJson(const nlohmann::json& value)
{
    notifyModified(); ++mDelta;
    mJsonPtr = std::make_unique<nlohmann::json>(value); // copy
}

/*****************************************************/
void JsonType::SetJson(std::nullptr_t)
{
    notifyModified(); ++mDelta;
    mJsonPtr = nullptr; // delete
}

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

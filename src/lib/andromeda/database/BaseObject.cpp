
#include <algorithm>

#include "BaseObject.hpp"
#include "MixedValue.hpp"
#include "ObjectDatabase.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
BaseObject::BaseObject(ObjectDatabase& database) :
    mDatabase(database), mDebug(__func__,this), mIdField("id", *this)
{
    MDBG_INFO("()");
    RegisterFields({&mIdField});
}

/*****************************************************/
void BaseObject::RegisterFields(const FieldList& list)
{
    for (FieldTypes::BaseField* field : list)
        mFields.emplace(field->GetName(), *field);
}

/*****************************************************/
void BaseObject::InitializeFields(const MixedParams& data)
{
    for (const MixedParams::value_type& pair : data)
    {
        MDBG_INFO("... " << pair.first << ":" << pair.second.ToString());
        mFields.at(pair.first).InitDBValue(pair.second);
    }
}

/*****************************************************/
void BaseObject::InitializeID(size_t len)
{
    mIdField.SetValue(StringUtil::Random(len));
    OBJDBG_INFO("()"); // AFTER ID is set
}

/*****************************************************/
const std::string& BaseObject::ID() const
{
    return mIdField.GetValue();
}

/*****************************************************/
BaseObject::operator std::string() const
{
    return ID()+":"+GetClassName();
}

/*****************************************************/
bool BaseObject::isModified() const
{
    return std::any_of(mFields.cbegin(), mFields.cend(), 
        [&](const decltype(mFields)::value_type& pair){ 
            return pair.second.isModified(); });
}

/*****************************************************/
void BaseObject::SetUnmodified()
{
    for (FieldMap::value_type& pair : mFields)
        pair.second.SetUnmodified();
}

/*****************************************************/
void BaseObject::Save()
{
    OBJDBG_INFO("()");

    FieldList fields;
    for (const FieldMap::value_type& pair : mFields)
    {
        if (pair.second.isModified())
            fields.emplace_back(&pair.second);
    }

    mDatabase.SaveObject(*this, fields);
}

} // namespace Database
} // namespace Andromeda

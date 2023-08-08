
#include "BaseField.hpp"
#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/ObjectDatabase.hpp"

namespace Andromeda {
namespace Database {
namespace FieldTypes {

/*****************************************************/
BaseField::BaseField(const char* name, BaseObject& parent, int delta) :
    mName(name), mDelta(delta), mParent(parent), mDatabase(parent.GetDatabase()) { }

/*****************************************************/
void BaseField::notifyModified()
{
    mDatabase.notifyModified(mParent);
}

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

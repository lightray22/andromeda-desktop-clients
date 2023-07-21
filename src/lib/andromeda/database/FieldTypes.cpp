
#include "BaseObject.hpp"
#include "FieldTypes.hpp"
#include "ObjectDatabase.hpp"

namespace Andromeda {
namespace Database {
namespace FieldTypes {

/*****************************************************/
BaseField::BaseField(const char* name, BaseObject& parent, size_t delta) :
    mName(name), mDelta(delta), mParent(parent), mDatabase(parent.GetDatabase()) { }

/*****************************************************/
void BaseField::notifyModified()
{
    mDatabase.notifyModified(mParent);
}

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda

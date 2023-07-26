#ifndef LIBA2_DATABASEEXCEPTION_H_
#define LIBA2_DATABASEEXCEPTION_H_

#include <string>

#include "andromeda/BaseException.hpp"

namespace Andromeda {
namespace Database {

/** Base Exception for database issues */
class DatabaseException : public BaseException { public:
    /** @param message error message */
    explicit DatabaseException(const std::string& message) :
        BaseException("Database Error: "+message) {}; };

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_DATABASEEXCEPTION_H_

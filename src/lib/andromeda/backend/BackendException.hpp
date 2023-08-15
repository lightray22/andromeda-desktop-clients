#ifndef LIBA2_BACKENDEXCEPTION_H_
#define LIBA2_BACKENDEXCEPTION_H_

#include <string>

#include "andromeda/BaseException.hpp"

namespace Andromeda {
namespace Backend {

/** Base Exception for database issues */
class BackendException : public BaseException { public:
    /** @param message error message */
    explicit BackendException(const std::string& message) :
        BaseException("Backend Error: "+message) {}; };

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_BACKENDEXCEPTION_H_

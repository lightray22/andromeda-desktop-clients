#ifndef LIBA2_BASEEXCEPTION_H_
#define LIBA2_BASEEXCEPTION_H_

#include <stdexcept>

namespace Andromeda {

/** Base Exception for all Andromeda errors */
class BaseException : public std::runtime_error 
{ 
    using std::runtime_error::runtime_error; 
};

} // namespace Andromeda

#endif // LIBA2_BASEEXCEPTION_H_

#ifndef A2GUI_OPTIONS_H_
#define A2GUI_OPTIONS_H_

#include <string>

#include "andromeda/BaseOptions.hpp"

namespace AndromedaGui {

/** Manages command line options and config */
class Options : public Andromeda::BaseOptions
{
public:

    /** Retrieve the standard help text string */
    static std::string HelpText();
};

} // namespace AndromedaGui

#endif // A2GUI_OPTIONS_H_

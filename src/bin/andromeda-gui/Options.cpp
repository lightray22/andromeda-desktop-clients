
#include <sstream>

#include "Options.hpp"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;

namespace AndromedaGui {

/*****************************************************/
std::string Options::HelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Usage Syntax: " << endl
        << "andromeda-gui " << CoreBaseHelpText() << endl << endl
           
        << OtherBaseHelpText() << endl;

    return output.str();
}

} // namespace AndromedaGui

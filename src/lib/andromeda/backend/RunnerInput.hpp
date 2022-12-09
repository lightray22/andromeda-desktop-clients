#ifndef LIBA2_RUNNERINPUT_H_
#define LIBA2_RUNNERINPUT_H_

#include <chrono>
#include <map>

namespace Andromeda {
namespace Backend {

/** API app-action call parameters */
struct RunnerInput
{
    /** An API file input param */
    struct File
    {
        /** File name */ std::string name;
        /** File data */ std::string data;
    };

    /** A map of input parameter key to string value */
    typedef std::map<std::string, std::string> Params;

    /** A map of input parameter key to input file */
    typedef std::map<std::string, File> Files;

    /** app name to run */     std::string app;
    /** app action to run */   std::string action;
    /** map of input params */ Params params = {};
    /** map of input files */  Files files = {};
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_RUNNERINPUT_H_

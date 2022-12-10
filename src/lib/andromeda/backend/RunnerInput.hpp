#ifndef LIBA2_RUNNERINPUT_H_
#define LIBA2_RUNNERINPUT_H_

#include <chrono>
#include <map>
#include <istream>

namespace Andromeda {
namespace Backend {

/** API app-action call parameters */
struct RunnerInput
{
    /** An API file input param */
    struct FileData
    {
        /** File name */ const std::string name;
        /** File data */ const std::string& data;
    };

    /** An API file stream input param */
    struct FileStream
    {
        /** File name */ const std::string name;
        /** File data */ std::istream& data;
    };

    /** A map of input parameter key to string value */
    typedef std::map<std::string, std::string> Params;

    /** A map of input parameter key to input file data */
    typedef std::map<std::string, FileData> FileDatas;

    /** A map of input parameter key to input file streams */
    typedef std::map<std::string, FileStream> FileStreams;

    /** app name to run */      std::string app;
    /** app action to run */    std::string action;
    /** map of input params */  Params params = {};
    /** map of input files */   FileDatas files = {};
    /** map of input streams */ FileStreams sfiles {};
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_RUNNERINPUT_H_

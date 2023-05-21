#ifndef LIBA2_RUNNERINPUT_H_
#define LIBA2_RUNNERINPUT_H_

#include <functional>
#include <istream>
#include <map>
#include <string>

#include "andromeda/BaseException.hpp"

namespace Andromeda {
namespace Backend {

/** API app-action call parameters */
struct RunnerInput
{
    /** Indicates an inability to seek the stream offset */
    class StreamSeekException : public BaseException { public:
        explicit StreamSeekException() : BaseException("Stream Seek Failure") {}; };

    /** Indicates that the stream was marked as failed */
    class StreamFailException : public BaseException { public:
        explicit StreamFailException() : BaseException("Stream Failure") {}; };

    /** A map of input parameter key to string value */
    typedef std::map<std::string, std::string> Params;

    /** app name to run */      std::string app;
    /** app action to run */    std::string action;

    /** map of non-sensitive and non-binary input params 
     * that can go in a URL (HTTP) or on the command line (CLI) */ 
    Params plainParams = {};
    
    /** map of sensitive or binary input params only to go
     * in headers/post body (HTTP) or environment vars (CLI) */
    Params dataParams = {};
};

/** A RunnerInput with strings for files input */
struct RunnerInput_FilesIn : RunnerInput
{
    /** An API file input param */
    struct FileData
    {
        /** File name */ const std::string name;
        /** File data */ const std::string& data;
    };

    /** A map of input parameter key to input file data */
    typedef std::map<std::string, FileData> FileDatas;
    /** map of input files */ FileDatas files = {};
};

/** 
 * A function to provides a buffer to write data into
 * MUST NOT call another backend action within the callback!
 * @param offset offset of the input data to send (may reset!)
 * @param buf pointer to buffer for data to be copied into
 * @param buflen max size of the buffer
 * @param written output number of bytes actually written
 * @return bool true if more data is remaining
 */
typedef std::function<bool(const size_t offset, char* const buf, const size_t buflen, size_t& written)> WriteFunc;

/** A RunnerInput with streams for files input */
struct RunnerInput_StreamIn : RunnerInput_FilesIn
{
    /** A combination of a file name and data Func for input */
    struct FileStream 
    { 
        const std::string name;
        const WriteFunc streamer;
    };

    /** Map of file streams to the input param name */
    typedef std::map<std::string, FileStream> FileStreams;
    FileStreams fstreams = {};

    /** Returns a Func that reads from the input string */
    static WriteFunc FromString(const std::string& data);
    /** Returns a Func that reads from the data stream */
    static WriteFunc FromStream(std::istream& data);
    /** Reads through a stream to find its size (ugly) */
    static size_t StreamSize(const WriteFunc& func);
};

/** 
 * A function that supplies a buffer to read output data out of
 * MUST NOT call another backend action within the callback!
 * @param offset offset of the current output data 
 * @param buf pointer to buffer containing data
 * @param buflen length of the data buffer
 */
typedef std::function<void(const size_t offset, const char* buf, const size_t buflen)> ReadFunc;

/** A RunnerInput with a function to stream output */
struct RunnerInput_StreamOut : RunnerInput
{
    /** The single output handler for the request */
    ReadFunc streamer;

    /** Returns a Func that writes to the data stream */
    static ReadFunc ToStream(std::ostream& data);
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_RUNNERINPUT_H_

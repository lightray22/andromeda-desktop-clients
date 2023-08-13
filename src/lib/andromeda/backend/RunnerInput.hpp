#ifndef LIBA2_RUNNERINPUT_H_
#define LIBA2_RUNNERINPUT_H_

#include <functional>
#include <istream>
#include <map>
#include <string>

#include "BackendException.hpp"

namespace Andromeda {
namespace Backend {

/** API app-action call parameters */
struct RunnerInput
{
    /** Indicates that the stream was marked as failed */
    class StreamFailException : public BackendException { public:
        explicit StreamFailException() : BackendException("Stream Failure") {};
        explicit StreamFailException(const std::string& msg) : BackendException("Stream Failure: "+msg) {}; };

    /** Indicates an inability to seek the stream offset */
    class StreamSeekException : public StreamFailException { public:
        explicit StreamSeekException() : StreamFailException("Seek") {}; };

    /** A map of input parameter key to string value */
    using Params = std::map<std::string, std::string>;

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
    using FileDatas = std::map<std::string, FileData>;
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
using WriteFunc = std::function<bool (const size_t, char *const, const size_t, size_t&)>;

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
    using FileStreams = std::map<std::string, FileStream>;
    FileStreams fstreams = {};

    /** Returns a Func that reads from the input string */
    static WriteFunc FromString(const std::string& data);
    /** 
     * Returns a Func that reads from the data stream 
    * @throws StreamFailException (the returned func)
     */
    static WriteFunc FromStream(std::istream& data);
    /** Reads through a stream to find its size (must be seekable) */
    static size_t StreamSize(const WriteFunc& func);
};

/** 
 * A function that supplies a buffer to read output data out of
 * MUST NOT call another backend action within the callback!
 * @param offset offset of the current output data 
 * @param buf pointer to buffer containing data
 * @param buflen length of the data buffer
 */
using ReadFunc = std::function<void (const size_t, const char*, const size_t)>;

/** A RunnerInput with a function to stream output */
struct RunnerInput_StreamOut : RunnerInput
{
    /** The single output handler for the request */
    ReadFunc streamer;

    /** 
     * Returns a Func that writes to the data stream
     * @throws StreamFailException (the returned func)
     */
    static ReadFunc ToStream(std::ostream& data);
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_RUNNERINPUT_H_

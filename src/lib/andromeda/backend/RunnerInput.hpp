#ifndef LIBA2_RUNNERINPUT_H_
#define LIBA2_RUNNERINPUT_H_

#include <functional>
#include <istream>
#include <map>
#include <string>

#include "BackendImpl.hpp"

namespace Andromeda {
namespace Backend {

/** API app-action call parameters */
struct RunnerInput
{
    /** Indicates an inability to seek the stream offset */
    class StreamSeekException : public BackendImpl::Exception { public:
        explicit StreamSeekException() : Exception("Stream Seek Failure") {}; };

    /** Indicates that the stream was marked as failed */
    class StreamFailException : public BackendImpl::Exception { public:
        explicit StreamFailException() : Exception("Stream Failure") {}; };

    /** A map of input parameter key to string value */
    typedef std::map<std::string, std::string> Params;

    /** app name to run */      std::string app;
    /** app action to run */    std::string action;
    /** map of input params */  Params params = {};
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

/** A RunnerInput with streams for files input */
struct RunnerInput_StreamIn : RunnerInput_FilesIn
{
    /** 
     * A function to provides a buffer to write data into
     * @param offset offset of the input data to send
     * @param buf pointer to buffer for data to be copied into
     * @param length max size of the buffer
     * @param read output number of bytes actually read
     * @return bool true if more data is remaining
     */
    typedef std::function<bool(const size_t offset, char* const buf, const size_t length, size_t& read)> WriteFunc;

    /** A combination of a file name and data Func for input */
    struct FileStream 
    { 
        const std::string name;
        const WriteFunc streamer;
    };

    /** Map of file streams to the input param name */
    typedef std::map<std::string, FileStream> FileStreams;
    FileStreams fstreams;

    /** Returns a Func that reads from the data stream */
    static WriteFunc FromStream(std::istream& data);
};

/** A RunnerInput with a function to stream output */
struct RunnerInput_StreamOut : RunnerInput
{
    /** 
     * A function that provides a buffer to read data out of 
     * @param offset offset of the current output data
     * @param data pointer to buffer containing data
     * @param length length of the data buffer
     */
    typedef std::function<void(const size_t offset, const char* buf, const size_t length)> ReadFunc;
    /** The single output handler for the request */
    ReadFunc streamer;

    /** Returns a Func that writes to the data stream */
    static ReadFunc ToStream(std::ostream& data);
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_RUNNERINPUT_H_

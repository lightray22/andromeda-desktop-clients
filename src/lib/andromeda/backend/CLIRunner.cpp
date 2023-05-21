
#include <filesystem>
#include <iostream>
#include <list>
#include <sstream>
#include <utility>

#include "reproc++/reproc.hpp"
#include "reproc++/drain.hpp"
#include "reproc++/fill.hpp"

#include "CLIRunner.hpp"
#include "RunnerInput.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
CLIRunner::CLIRunner(const std::string& apiPath, const RunnerOptions& runnerOptions) :
    mDebug(__func__,this), mApiPath(FixApiPath(apiPath)), mOptions(runnerOptions)
{
    MDBG_INFO("(apiPath:" << mApiPath << ")");
}

/*****************************************************/
std::unique_ptr<BaseRunner> CLIRunner::Clone() const
{
    return std::make_unique<CLIRunner>(mApiPath, mOptions);
}

/*****************************************************/
std::string CLIRunner::FixApiPath(std::string apiPath)
{
    MDBG_INFO("(apiPath:" << apiPath << ")");

    if (apiPath.empty()) apiPath = "andromeda-server";
    else if (std::filesystem::is_directory(apiPath))
        apiPath += "/andromeda-server";
    return apiPath;
}

/*****************************************************/
void CLIRunner::CheckError(reproc::process& process, const std::error_code& error)
{
    if (error) {
        process.terminate();
        throw Exception(error.message()); 
    }
}

/*****************************************************/
CLIRunner::ArgList CLIRunner::GetArguments(const RunnerInput& input)
{
    ArgList arguments { mApiPath, "--json", input.app, input.action };

    if (Utilities::endsWith(mApiPath, ".php"))
        arguments.emplace_front("php");

    for (const RunnerInput::Params::value_type& param : input.plainParams)
    {
        arguments.push_back("--"+param.first);
        arguments.push_back(param.second);
    }

    return arguments;
}

/*****************************************************/
CLIRunner::EnvList CLIRunner::GetEnvironment(const RunnerInput& input)
{
    EnvList environment;
    for (const RunnerInput::Params::value_type& param : input.dataParams)
        environment.emplace("andromeda_"+param.first, param.second);
    return environment;
}

/*****************************************************/
void CLIRunner::PrintArgs(const CLIRunner::ArgList& argList)
{
    mDebug.Info([&](std::ostream& str) {
        for (const std::string& arg : argList) 
            str << arg << " ";
    });
}

/*****************************************************/
void CLIRunner::StartProc(reproc::process& process, const ArgList& args, const EnvList& env)
{
    reproc::options options; 
    options.env.extra = env;
    CheckError(process, process.start(args, options));
}

/*****************************************************/
void CLIRunner::DrainProc(reproc::process& process, std::string& output)
{
    reproc::sink::string sink(output);
    CheckError(process, reproc::drain(process, sink, 
        reproc::sink::null, mOptions.streamBufferSize));
}

/*****************************************************/
int CLIRunner::FinishProc(reproc::process& process)
{
    int status = 0; 
    std::error_code error; 
    std::tie(status,error) = process.wait(mOptions.timeout); 
    CheckError(process, error);
    return status;
}

/*****************************************************/
std::string CLIRunner::RunAction_Write(const RunnerInput& input)
{
    MDBG_INFO("()");

    const ArgList arguments { GetArguments(input) }; 
    const EnvList environment { GetEnvironment(input) };
    PrintArgs(arguments);

    reproc::process process;
    StartProc(process, arguments, environment);

    std::string output;
    DrainProc(process, output);
    FinishProc(process);
    return output;
}

/*****************************************************/
std::string CLIRunner::RunAction_FilesIn(const RunnerInput_FilesIn& input)
{
    MDBG_INFO("()");

    ArgList arguments { GetArguments(input) };
    const EnvList environment { GetEnvironment(input) };

    const std::string* inputPtr { nullptr };
    if (input.files.size() > 0)
    {
        if (input.files.size() > 1) throw Exception("Multiple Files");
        const decltype(input.files)::value_type& infile(*(input.files.begin()));

        arguments.push_back("--"+infile.first+"-");
        arguments.push_back(infile.second.name);
        inputPtr = &(infile.second.data);
    }

    PrintArgs(arguments);
    
    reproc::process process;
    StartProc(process, arguments, environment);

    std::error_code fillErr;
    if (inputPtr != nullptr)
    {
        reproc::filler::string filler(*inputPtr);
        fillErr = reproc::fill(process, filler, mOptions.streamBufferSize);
        process.close(reproc::stream::in);
    }

    std::string output;
    DrainProc(process, output);
    int status { FinishProc(process) };

    // if we got a fill error and the process claimed to succeed, throw
    if (!status && fillErr) CheckError(process, fillErr);

    return output;
}

/*****************************************************/
std::string CLIRunner::RunAction_StreamIn(const RunnerInput_StreamIn& input)
{
    MDBG_INFO("()");

    ArgList arguments { GetArguments(input) };
    const EnvList environment { GetEnvironment(input) };

    if (input.files.size() > 0) throw Exception("Multiple Files");

    const WriteFunc* streamerPtr { nullptr };
    if (input.fstreams.size() > 0)
    {
        if (input.fstreams.size() > 1) throw Exception("Multiple Files");
        const decltype(input.fstreams)::value_type& instream(*(input.fstreams.begin()));

        arguments.push_back("--"+instream.first+"-");
        arguments.push_back(instream.second.name);
        streamerPtr = &instream.second.streamer;
    }

    PrintArgs(arguments);
    
    reproc::process process;
    StartProc(process, arguments, environment);

    std::error_code fillErr;
    if (streamerPtr != nullptr)
    {
        size_t offset { 0 }; fillErr = reproc::fill(process, 
            [&](uint8_t* const buffer, const size_t bufSize, size_t& written, bool& more)->std::error_code
        {
            more = (*streamerPtr)(offset, reinterpret_cast<char*>(buffer), bufSize, written);
            offset += written; return std::error_code(); // success
        }, mOptions.streamBufferSize);
        process.close(reproc::stream::in);
    }

    std::string output;
    DrainProc(process, output);
    int status { FinishProc(process) };

    // if we got a fill error and the process claimed to succeed, throw
    if (!status && fillErr) CheckError(process, fillErr);

    return output;
}

/*****************************************************/
void CLIRunner::RunAction_StreamOut(const RunnerInput_StreamOut& input)
{
    MDBG_INFO("()");

    const ArgList arguments { GetArguments(input) };
    const EnvList environment { GetEnvironment(input) };
    PrintArgs(arguments);
    
    reproc::process process;
    StartProc(process, arguments, environment);

    size_t offset { 0 }; CheckError(process, reproc::drain(process, 
        [&](reproc::stream stream, const uint8_t* buffer, size_t size)->std::error_code
    {
        input.streamer(offset, reinterpret_cast<const char*>(buffer), size);
        offset += size; return std::error_code(); // success
    }, reproc::sink::null, mOptions.streamBufferSize));

    FinishProc(process);
}

} // namespace Backend
} // namespace Andromeda

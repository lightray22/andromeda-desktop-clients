
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <list>
#include <sstream>
#include <utility>
#include <vector>

#include "reproc++/reproc.hpp"
#include "reproc++/drain.hpp"
#include "reproc++/fill.hpp"

#if !WIN32
#include <string.h> // // NOLINT(*-deprecated-headers)
#include <unistd.h> // execvp
#include <sys/types.h> // pid_t
#include <sys/wait.h> // waitpid
#endif // !WIN32

#include "CLIRunner.hpp"
#include "RunnerInput.hpp"
#include "andromeda/StringUtil.hpp"

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

    if (StringUtil::endsWith(mApiPath, ".php"))
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

// TODO implement retries for CLI (can get 503's)

/*****************************************************/
void CLIRunner::StartProc(reproc::process& process, const ArgList& args, const EnvList& env)
{
    reproc::options options; 
    options.env.extra = env;
    CheckError(process, process.start(args, options));
}

/*****************************************************/
void CLIRunner::DrainProc(reproc::process& process, std::string& output, const size_t bufferSize)
{
    reproc::sink::string sink(output);
    CheckError(process, reproc::drain(process, sink, 
        reproc::sink::null, bufferSize));
}

/*****************************************************/
int CLIRunner::FinishProc(reproc::process& process, const std::chrono::milliseconds& timeout)
{
    int status = 0; 
    std::error_code error; 
    std::tie(status,error) = process.wait(timeout); 
    CheckError(process, error);
    return status;
}

/*****************************************************/
// TODO see https://github.com/DaanDeMeyer/reproc/issues/106
/*int CLIRunner::RunCommand(const ArgList& args)
{
    const EnvList env; // empty
    reproc::process process;
    StartProc(process, args, env);
    return FinishProc(process, reproc::infinite);
}*/

#if !WIN32
/*****************************************************/
int CLIRunner::RunPosixCommand(ArgList& args)
{
    if (args.empty()) throw CmdException("empty argument list");

    std::vector<char*> argv(args.size());
    std::transform(args.begin(),args.end(),argv.begin(),
        [](std::string& arg)->char*{ return arg.data(); });
    argv.emplace_back(nullptr); // argv is null-terminated

    const pid_t pid { fork() };
    if (pid < 0) throw CmdException(
        std::string("fork: ")+strerror(errno)); // NOLINT(concurrency-mt-unsafe)

    if (pid == 0) // child
    {
        execvp(argv[0], argv.data()); // only returns on error
        _exit(errno < 128 ? 128+errno : 128);
    }

    int status = -1;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
    {
        const int code { WEXITSTATUS(status) };
        if (code >= 128) throw CmdException(std::string("exec: ")
            +std::to_string(code-128)+" "+strerror(code-128)); // NOLINT(concurrency-mt-unsafe)
        else return code;
    }
    else if (WIFSIGNALED(status))
    {
        const int sig { WTERMSIG(status) };
        throw CmdException(std::string("signal: ")
            +std::to_string(sig)+" "+strsignal(sig)); // NOLINT(concurrency-mt-unsafe)
    }
    else throw CmdException("unknown status");
}
#endif // !WIN32

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
    DrainProc(process, output, mOptions.streamBufferSize);
    FinishProc(process, mOptions.timeout);
    return output;
}

/*****************************************************/
std::string CLIRunner::RunAction_FilesIn(const RunnerInput_FilesIn& input)
{
    MDBG_INFO("()");

    ArgList arguments { GetArguments(input) };
    const EnvList environment { GetEnvironment(input) };

    const std::string* inputPtr { nullptr };
    if (!input.files.empty())
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
    DrainProc(process, output, mOptions.streamBufferSize);
    const int status { FinishProc(process, mOptions.timeout) };

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

    if (!input.files.empty()) throw Exception("Multiple Files");

    const WriteFunc* streamerPtr { nullptr };
    if (!input.fstreams.empty())
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
    DrainProc(process, output, mOptions.streamBufferSize);
    const int status { FinishProc(process, mOptions.timeout) };

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
        // TODO see server issue with output modes... should we check if the stream here is out of err or both??
        input.streamer(offset, reinterpret_cast<const char*>(buffer), size);
        offset += size; return std::error_code(); // success
    }, reproc::sink::null, mOptions.streamBufferSize));

    FinishProc(process, mOptions.timeout);
}

} // namespace Backend
} // namespace Andromeda

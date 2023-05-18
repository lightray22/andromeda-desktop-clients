
#include <filesystem>
#include <iostream>
#include <list>
#include <sstream>
#include <utility>

#include "reproc++/reproc.hpp"
#include "reproc++/drain.hpp"

#include "CLIRunner.hpp"
#include "RunnerInput.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
CLIRunner::CLIRunner(const std::string& apiPath, const std::chrono::seconds& timeout) :
    mDebug(__func__,this), mApiPath(FixApiPath(apiPath)), mTimeout(timeout)
{
    MDBG_INFO("(apiPath:" << mApiPath << ")");
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
std::unique_ptr<BaseRunner> CLIRunner::Clone() const
{
    return std::make_unique<CLIRunner>(mApiPath, mTimeout);
}

/*****************************************************/
std::string CLIRunner::RunAction(const RunnerInput& input)
{
    std::list<std::string> arguments { mApiPath, "--json", input.app, input.action };

    if (Utilities::endsWith(mApiPath, ".php"))
        arguments.emplace_front("php");

    for (const RunnerInput::Params::value_type& param : input.params)
    {
        arguments.push_back("--"+param.first);
        arguments.push_back(param.second);
    }

    std::ostringstream command; 
    for (const std::string& str : arguments) 
        command << str << " ";

    MDBG_INFO("... command:" << command.str());
    
    std::error_code error; reproc::process process;
    error = process.start(arguments);
    if (error) throw Exception(error.message());

    std::string output; reproc::sink::string sink(output);
    error = reproc::drain(process, sink, reproc::sink::null);

    if (error) { process.terminate(); throw Exception(error.message()); }

    int status = 0; std::tie(status,error) = process.wait(mTimeout);

    if (error) { process.terminate(); throw Exception(error.message()); }

    return output;
}

/*****************************************************/
std::string CLIRunner::RunAction(const RunnerInput_FilesIn& input)
{
    throw Exception("not implemented 1");
    // TODO implement me (commonize above)


    /*const std::string* inputPtr { nullptr };

    if (input.files.size() > 0) // TODO move to FilesIn
    {
        if (input.files.size() > 1) throw Exception("Multiple Files");
        const RunnerInput::FileDatas::value_type& infile(*(input.files.begin()));

        arguments.push_back("--"+infile.first+"-");
        inputPtr = &(infile.second.data);
    }

    // TODO command, process start go here

    if (inputPtr != nullptr)
    {
        const auto* inputData { reinterpret_cast<const uint8_t*>(inputPtr->c_str()) };

        reproc::input procIn(inputData, inputPtr->size());
        error = reproc::fill(process, procIn);

        if (error) { process.terminate(); throw Exception(error.message()); }
    }*/


}

/*****************************************************/
std::string CLIRunner::RunAction(const RunnerInput_StreamIn& input)
{
    throw Exception("not implemented 2");
    // TODO implement me (commonize above)
}

/*****************************************************/
void CLIRunner::RunAction(const RunnerInput_StreamOut& input)
{
    throw Exception("not implemented 3");
    // TODO implement me (commonize above)
}

} // namespace Backend
} // namespace Andromeda

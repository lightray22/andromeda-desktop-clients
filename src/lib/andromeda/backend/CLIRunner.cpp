
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
CLIRunner::CLIRunner(const std::string& apiPath) :
    mDebug(__func__,this), mApiPath(apiPath)
{
    if (mApiPath.empty()) mApiPath = "andromeda-server";
    else if (std::filesystem::is_directory(mApiPath))
        mApiPath += "/andromeda-server";

    MDBG_INFO("(apiPath:" << mApiPath << ")");
}

/*****************************************************/
std::unique_ptr<BaseRunner> CLIRunner::Clone()
{
    return std::make_unique<CLIRunner>(mApiPath);
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

    const std::string* inputPtr { nullptr };

    /*if (input.files.size() > 0)
    {
        if (input.files.size() > 1) throw Exception("Multiple Files");
        const RunnerInput::FileDatas::value_type& infile(*(input.files.begin()));

        arguments.push_back("--"+infile.first+"-");
        inputPtr = &(infile.second.data);
    }*/ // TODO move to FilesIn

    std::ostringstream command; 
    for (const std::string& str : arguments) 
        command << str << " ";

    MDBG_INFO("... command:" << command.str());
    
    std::error_code error; reproc::process process;
    error = process.start(arguments);
    if (error) throw Exception(error.message());

    if (inputPtr != nullptr)
    {
        const auto* inputData { reinterpret_cast<const uint8_t*>(inputPtr->c_str()) };

        reproc::input procIn(inputData, inputPtr->size());
        error = reproc::fill(process, procIn);

        if (error) { process.terminate(); throw Exception(error.message()); }
    }

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
    throw Exception("not implemented");
    // TODO implement me (commonize above)
}

/*****************************************************/
std::string CLIRunner::RunAction(const RunnerInput_StreamIn& input)
{
    throw Exception("not implemented");
    // TODO implement me (commonize above)
}

void CLIRunner::RunAction(const RunnerInput_StreamOut& input)
{
    throw Exception("not implemented");
    // TODO implement me (commonize above)
}

} // namespace Backend
} // namespace Andromeda

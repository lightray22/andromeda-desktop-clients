
#include <iostream>
#include <list>
#include <utility>

#include "reproc++/reproc.hpp"
#include "reproc++/drain.hpp"

#include "CLIRunner.hpp"

namespace Andromeda {

/*****************************************************/
CLIRunner::CLIRunner(const std::string& apiPath) :
    debug("CLIRunner",this), apiPath(apiPath)
{
    if (apiPath.empty()) this->apiPath = "andromeda-server";

    else if (!Utilities::endsWith(this->apiPath, "/index.php") &&
             !Utilities::endsWith(this->apiPath, "/andromeda-server")) 
        this->apiPath += "/andromeda-server";

    debug << __func__ << "(apiPath:" << this->apiPath << ")"; debug.Info();
}

/*****************************************************/
std::string CLIRunner::RunAction(const Backend::Runner::Input& input)
{
    std::list<std::string> arguments { this->apiPath, "--json", input.app, input.action };

    if (Utilities::endsWith(this->apiPath, "index.php"))
        arguments.emplace_front("php");

    for (const Params::value_type& param : input.params)
    {
        arguments.push_back("--"+param.first);
        arguments.push_back(param.second);
    }

    const std::string* inputPtr = nullptr;

    if (input.files.size() > 0)
    {
        if (input.files.size() > 1) throw Exception("Multiple Files");
        const Files::value_type& infile(*(input.files.begin()));

        arguments.push_back("--"+infile.first+"-");
        inputPtr = &(infile.second.data);
    }

    std::ostringstream command; 
    for (const std::string& str : arguments) 
        command << str << " ";

    debug << __func__ << "()... command:" << command.str(); debug.Info();
    
    std::error_code error; reproc::process process;
    error = process.start(arguments);
    if (error) throw Exception(error.message());

    if (inputPtr != nullptr)
    {
        const auto* inputData = reinterpret_cast<const uint8_t*>(inputPtr->c_str());

        reproc::input procIn(inputData, inputPtr->size());
        error = reproc::fill(process, procIn);

        if (error) { process.terminate(); throw Exception(error.message()); }
    }

    std::string output; reproc::sink::string sink(output);
    error = reproc::drain(process, sink, reproc::sink::null);

    if (error) { process.terminate(); throw Exception(error.message()); }

    int status = 0; std::tie(status,error) = process.wait(this->timeout);

    if (error) { process.terminate(); throw Exception(error.message()); }

    return output;
}

} // namespace Andromeda


#include <vector>
#include <utility>
#include <iostream>

#include "reproc++/run.hpp"

#include "CLIRunner.hpp"

/*****************************************************/
CLIRunner::CLIRunner(const std::string& apiPath) :
    debug("CLIRunner",this), apiPath(apiPath)
{
    debug << __func__ << "(apiPath:" << this->apiPath << ")"; debug.Info();
}

/*****************************************************/
std::string CLIRunner::RunAction(const Backend::Runner::Input& input)
{
    std::vector<std::string> arguments { this->apiPath, "--json", input.app, input.action };

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
    if (error) throw Exception(error);

    if (inputPtr != nullptr)
    {
        size_t written = 0; size_t size = inputPtr->size();
        
        std::tie(written,error) =
            process.write((const uint8_t*)(inputPtr->c_str()), size);
        
        if (error) throw Exception(error);
        else if (written != size)
        {
            std::ostringstream err; err << "Wrote " << written << " of " << size;
            throw Exception(err.str());
        }

        error = process.close(reproc::stream::in);
        if (error) throw Exception(error);
    }

    std::string output; reproc::sink::string sink(output);
    error = reproc::drain(process, sink, reproc::sink::null);
    if (error) throw Exception(error);

    int status = 0; std::tie(status,error) = process.wait(this->timeout);
    if (error) throw Exception(error); 
    if (status) throw Exception(std::to_string(status));

    return output;
}

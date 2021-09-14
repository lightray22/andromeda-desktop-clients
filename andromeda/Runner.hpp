#ifndef LIBA2_RUNNER_H_
#define LIBA2_RUNNER_H_

#include <string>
#include <stdexcept>

class RunnerException : public std::runtime_error { 
    using std::runtime_error::runtime_error; };

class APIErrorException : public RunnerException { 
    public: 
    APIErrorException() : 
        RunnerException("unknown backend error") {};
    APIErrorException(std::string message) : 
        RunnerException("backend error: "+message) {}; };

class APINotFoundException : public APIErrorException {
    using APIErrorException::APIErrorException; };

class Runner
{
public:

    virtual ~Runner(){ };

    virtual std::string RunAction(const std::string& app, const std::string& action) = 0;
};

#endif
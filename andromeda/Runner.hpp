#ifndef LIBA2_RUNNER_H_
#define LIBA2_RUNNER_H_

#include <string>
#include "Utilities.hpp"

class Runner
{
public:

    class Exception : public AndromedaException { 
        using AndromedaException::AndromedaException; };

    virtual ~Runner(){ };

    virtual std::string RunAction(const std::string& app, const std::string& action) = 0;
};

#endif
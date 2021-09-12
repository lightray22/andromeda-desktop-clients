
#ifndef LIBA2_BACKEND_H_
#define LIBA2_BACKEND_H_

#include <string>

class Backend
{
public:

    Backend(const std::string& apiLocation, const std::string& username); // TODO maybe take a struct?

private:

    std::string apiLocation;
};

#endif
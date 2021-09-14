#include "Utilities.hpp"

std::vector<std::string> Utilities::explode(
    std::string str, const std::string& delim, 
    const int max, const size_t skip)
{
    size_t skipped = 0, start = 0, end;
    std::vector<std::string> retval;

    while ((end = str.find(delim, start)) != std::string::npos
            && (max < 0 || retval.size() < max - 1))
    {
        if (skipped >= skip)
        {
            start = 0;
            retval.push_back(str.substr(0, end));
            str.erase(0, end + delim.length());
        }
        else 
        {
            skipped++;
            start = (end + delim.length());
        }
    }

    retval.push_back(str);
    return std::move(retval);
}

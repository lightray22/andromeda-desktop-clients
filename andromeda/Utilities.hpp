
#ifndef LIBA2_UTILITIES_H_
#define LIBA2_UTILITIES_H_

#include <vector>
#include <string>
#include <utility>

class Utilities
{
public:
    template <class T>
    static std::string implode(T arr, const std::string& delim)
    {
        std::string retval;
        for (const std::string& piece : arr) 
            retval += piece;
        return std::move(retval);
    }

    static std::vector<std::string> explode(
        std::string str, const std::string& delim, 
        const int max = -1, const size_t skip = 0);
};

#endif
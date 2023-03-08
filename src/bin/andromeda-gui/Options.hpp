#ifndef A2GUI_OPTIONS_H_
#define A2GUI_OPTIONS_H_

#include <string>

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    namespace Filesystem { namespace Filedata { struct CacheOptions; } }
}

namespace AndromedaGui {

/** Manages command line options and config */
class Options : public Andromeda::BaseOptions
{
public:

    /** Retrieve the standard help text string */
    static std::string HelpText();

    /**
     * @param cacheOptions CacheManager options ref to fill
     */
    explicit Options(Andromeda::Filesystem::Filedata::CacheOptions& cacheOptions);

    virtual bool AddFlag(const std::string& flag) override;

    virtual bool AddOption(const std::string& option, const std::string& value) override;

private:

    Andromeda::Filesystem::Filedata::CacheOptions& mCacheOptions;
};

} // namespace AndromedaGui

#endif // A2GUI_OPTIONS_H_

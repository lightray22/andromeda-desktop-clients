#ifndef A2CLI_OPTIONS_H_
#define A2CLI_OPTIONS_H_

#include <string>

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    namespace Backend { struct HTTPOptions; struct RunnerOptions; }
}

namespace AndromedaCli {

/** Manages command line options and config */
class Options : public Andromeda::BaseOptions
{
public:

    /** Retrieve the base usage help text string */
    static std::string CoreHelpText();

    /** Retrieve the main command help text string */
    static std::string MainHelpText();

    /** Retrieve the detailed options help text string */
    static std::string DetailHelpText();

    /** 
     * @param[out] httpOptions HTTPRunner options ref to fill 
     * @param[out] runnerOptions BaseRunner options ref to fill
     */
    explicit Options(
        Andromeda::Backend::HTTPOptions& httpOptions,
        Andromeda::Backend::RunnerOptions& runnerOptions);

    virtual bool AddFlag(const std::string& flag) override;

    virtual bool AddOption(const std::string& option, const std::string& value) override;

    virtual void Validate() override;

    /** Returns the URL of the API endpoint */
    std::string GetApiUrl() const { return mApiUrl; }

    /** Returns true if output streaming is requested */
    bool isStreamOut() const { return mStreamOut; }

    /** Returns true if unsafe URLs are allowed */
    bool AllowUnsafeUrl() const { return mUnsafeUrl; }

private:

    Andromeda::Backend::HTTPOptions& mHttpOptions;
    Andromeda::Backend::RunnerOptions& mRunnerOptions;

    std::string mApiUrl;
    bool mStreamOut { false };
    bool mUnsafeUrl { false };
};

} // namespace AndromedaCli

#endif // A2CLI_OPTIONS_H_

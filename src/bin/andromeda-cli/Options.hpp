#ifndef A2CLI_OPTIONS_H_
#define A2CLI_OPTIONS_H_

#include <string>

#include "andromeda/BaseOptions.hpp"
#include "andromeda/backend/HTTPOptions.hpp"

namespace AndromedaCli {

/** Manages command line options and config */
class Options : public Andromeda::BaseOptions
{
public:

    /** Retrieve the standard help text string */
    static std::string CoreHelpText();

    /** Retrieve the standard help text string */
    static std::string OtherHelpText();

    /** @param httpOptions HTTPRunner options ref to fill */
    explicit Options(Andromeda::Backend::HTTPOptions& httpOptions);

    virtual bool AddFlag(const std::string& flag) override;

    virtual bool AddOption(const std::string& option, const std::string& value) override;

    virtual void Validate() override;

    /** Returns the URL of the API endpoint */
    std::string GetApiUrl() const { return mApiUrl; }

private:

    Andromeda::Backend::HTTPOptions& mHttpOptions;

    std::string mApiUrl;
};

} // namespace AndromedaCli

#endif // A2CLI_OPTIONS_H_

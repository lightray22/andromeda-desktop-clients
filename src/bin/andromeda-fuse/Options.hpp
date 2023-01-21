#ifndef A2FUSE_OPTIONS_H_
#define A2FUSE_OPTIONS_H_

#include <string>

#include "andromeda-fuse/FuseAdapter.hpp"
#include "andromeda-fuse/FuseOptions.hpp"

#include "andromeda/BaseOptions.hpp"
#include "andromeda/backend/ConfigOptions.hpp"
#include "andromeda/backend/HTTPOptions.hpp"

namespace AndromedaFuse {

/** Manages command line options and config */
class Options : public Andromeda::BaseOptions
{
public:

    /** Retrieve the standard help text string */
    static std::string HelpText();

    /**
     * @param configOptions Config options ref to fill
     * @param httpOptions HTTPRunner options ref to fill
     * @param fuseOptions FUSE options ref to fill
     */
    Options(Andromeda::Backend::ConfigOptions& configOptions, 
            Andromeda::Backend::HTTPOptions& httpOptions, 
            AndromedaFuse::FuseOptions& fuseOptions);

    virtual bool AddFlag(const std::string& flag) override;

    virtual bool AddOption(const std::string& option, const std::string& value) override;

    virtual void TryAddUrlOption(const std::string& option, const std::string& value) override;

    virtual void Validate() override;

    /** Backend connection type */
    enum class ApiType
    {
        API_URL,
        API_PATH
    };

    /** Returns the specified API type */
    ApiType GetApiType() const { return mApiType; }    

    /** Returns the path to the API endpoint */
    std::string GetApiPath() const { return mApiPath; }

    /** Returns true if a username is specified */
    bool HasUsername() const { return !mUsername.empty(); }

    /** Returns the specified username */
    std::string GetUsername() const { return mUsername; }

    /** Returns true if a password is specified */
    bool HasPassword() const { return !mPassword.empty(); }

    /** Returns the specified password */
    std::string GetPassword() const { return mPassword; }

    /** Returns true if a session ID was provided */
    bool HasSession() const { return !mSessionid.empty(); }

    /** Returns the specified session ID */
    std::string GetSessionID() const { return mSessionid; }

    /** Returns the specified session key */
    std::string GetSessionKey() const { return mSessionkey; }

    /** Returns true if using a session is forced */
    bool GetForceSession() const { return mForceSession; }

    /** Folder types that can be mounted as root */
    enum class RootType
    {
        SUPERROOT,
        FILESYSTEM,
        FOLDER
    };

    /** Returns the filesystem directory to mount */
    const std::string& GetMountPath() const { return mMountPath; }

    /** Returns the specified mount item type */
    RootType GetMountRootType() const { return mMountRootType; }

    /** Returns the specified mount item ID */
    std::string GetMountItemID() const { return mMountItemID; }

    /** Returns true if we should run in the foreground */
    bool isForeground() const { return mForeground; }

private:

    Andromeda::Backend::ConfigOptions& mConfigOptions;
    Andromeda::Backend::HTTPOptions& mHttpOptions;
    AndromedaFuse::FuseOptions& mFuseOptions;

    ApiType mApiType { static_cast<ApiType>(-1) };
    std::string mApiPath;
    std::string mMountPath;

    std::string mUsername;
    std::string mPassword;
    bool mForceSession { false };

    std::string mSessionid;
    std::string mSessionkey;
    
    RootType mMountRootType { RootType::SUPERROOT };
    std::string mMountItemID;

    bool mForeground { false };
};

} // namespace AndromedaFuse

#endif // A2FUSE_OPTIONS_H_

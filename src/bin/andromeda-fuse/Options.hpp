#ifndef A2FUSE_OPTIONS_H_
#define A2FUSE_OPTIONS_H_

#include <string>

#include "andromeda-fuse/FuseAdapter.hpp"
#include "andromeda-fuse/FuseOptions.hpp"

#include "andromeda/BaseOptions.hpp"

namespace Andromeda {
    struct ConfigOptions;
    namespace Backend { struct HTTPOptions; struct RunnerOptions; }
    namespace Filesystem { namespace Filedata { struct CacheOptions; } }
}

namespace AndromedaFuse {

/** Manages command line options and config */
class Options : public Andromeda::BaseOptions
{
public:

    /** Retrieve the standard help text string */
    static std::string HelpText();

    /**
     * @param[out] configOptions Config options ref to fill
     * @param[out] httpOptions HTTPRunner options ref to fill
     * @param[out] runnerOptions BaseRunner options ref to fill
     * @param[out] cacheOptions CacheManager options ref to fill
     * @param[out] fuseOptions FUSE options ref to fill
     */
    Options(Andromeda::ConfigOptions& configOptions, 
            Andromeda::Backend::HTTPOptions& httpOptions, 
            Andromeda::Backend::RunnerOptions& runnerOptions,
            Andromeda::Filesystem::Filedata::CacheOptions& cacheOptions,
            AndromedaFuse::FuseOptions& fuseOptions);

    bool AddFlag(const std::string& flag) override;

    bool AddOption(const std::string& option, const std::string& value) override;

    void TryAddUrlOption(const std::string& option, const std::string& value) override;

    void Validate() override;

    /** Backend connection type */
    enum class ApiType
    {
        API_URL,
        API_PATH
    };

    /** Returns the specified API type */
    [[nodiscard]] ApiType GetApiType() const { return mApiType; }

    /** Returns the path to the API endpoint */
    [[nodiscard]] std::string GetApiPath() const { return mApiPath; }

    /** Returns true if a username is specified */
    [[nodiscard]] bool HasUsername() const { return !mUsername.empty(); }

    /** Returns the specified username */
    [[nodiscard]] std::string GetUsername() const { return mUsername; }

    /** Returns true if a password is specified */
    [[nodiscard]] bool HasPassword() const { return !mPassword.empty(); }

    /** Returns the specified password */
    [[nodiscard]] std::string GetPassword() const { return mPassword; }

    /** Returns true if a session ID was provided */
    [[nodiscard]] bool HasSession() const { return !mSessionid.empty(); }

    /** Returns the specified session ID */
    [[nodiscard]] std::string GetSessionID() const { return mSessionid; }

    /** Returns the specified session key */
    [[nodiscard]] std::string GetSessionKey() const { return mSessionkey; }

    /** Returns true if using a session is forced */
    [[nodiscard]] bool GetForceSession() const { return mForceSession; }

    /** Folder types that can be mounted as root */
    enum class RootType
    {
        SUPERROOT,
        FILESYSTEM,
        FOLDER
    };

    /** Returns the filesystem directory to mount */
    [[nodiscard]] const std::string& GetMountPath() const { return mMountPath; }

    /** Returns the specified mount item type */
    [[nodiscard]] RootType GetMountRootType() const { return mMountRootType; }

    /** Returns the specified mount item ID */
    [[nodiscard]] std::string GetMountItemID() const { return mMountItemID; }

    /** Returns true if we should run in the foreground */
    [[nodiscard]] bool isForeground() const { return mForeground; }

private:

    Andromeda::ConfigOptions& mConfigOptions;
    Andromeda::Backend::HTTPOptions& mHttpOptions;
    Andromeda::Backend::RunnerOptions& mRunnerOptions;
    Andromeda::Filesystem::Filedata::CacheOptions& mCacheOptions;
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

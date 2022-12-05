#ifndef A2GUI_MOUNTCONTEXT_H
#define A2GUI_MOUNTCONTEXT_H

#include <filesystem>
#include <memory>
#include <string>

#include "andromeda/Utilities.hpp"
#include "andromeda-fuse/FuseAdapter.hpp"

namespace Andromeda { class Backend; 
    namespace FSItems { class Folder; }
}

/** Encapsulates a FUSE mount and root folder */
class MountContext
{
public:

    /** Base Exception for mount context issues */
    class Exception : public Andromeda::Utilities::Exception { public:
        /** @param message error message */
        explicit Exception(const std::string& message) :
            Andromeda::Utilities::Exception("Mount Error: "+message) {}; };

    /** Exception indicating that no home directory was found */
    class UnknownHomeException : public Exception { public:
        UnknownHomeException() : Exception("Unknown Home Directory") {}; };

    /** Exception indicating the desired mount directory is not empty */
    class NonEmptyMountException : public Exception { public:
        explicit NonEmptyMountException(const std::string& path) : 
            Exception("Mount Directory not empty:\n\n"+path) {}; };

    /** Exception indicating std::filesystem threw an exception */
    class FilesystemErrorException : public Exception { public:
        explicit FilesystemErrorException(const std::filesystem::filesystem_error& err) : 
            Exception("Filesystem Error: "+std::string(err.what())) {}; };

    /**
     * Create a new MountContext
     * @param home if true, options.mountPath is home-relative
     * @param backend the backend resource to use
     * @param options FUSE adapter options
     */
    MountContext(bool home, Andromeda::Backend& backend, AndromedaFuse::FuseAdapter::Options& options);

    virtual ~MountContext();

    /** Returns the FUSE mount path */
    const std::string& GetMountPath() const;

private:

    /** Sets up and returns the path to HOMEDIR/Andromeda */
    const std::string& InitHomeRoot();

    /** The path to the user's HOMEDIR (if init'd) */
    std::string mHomeRoot;

    std::unique_ptr<Andromeda::FSItems::Folder> mRootFolder;
    std::unique_ptr<AndromedaFuse::FuseAdapter> mFuseAdapter;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_MOUNTCONTEXT_H

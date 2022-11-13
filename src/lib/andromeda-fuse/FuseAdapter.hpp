#ifndef A2FUSE_FUSEADAPTER_H_
#define A2FUSE_FUSEADAPTER_H_

#include <string>
#include <list>

#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace FSItems { 
    class Folder;
} // nanespace FSItems
} // namespace Andromeda

namespace AndromedaFuse {

/** Static class for FUSE operations */
class FuseAdapter
{
public:

    /** Base Exception for all FUSE issues */
    class Exception : public Andromeda::Utilities::Exception { public:
        /** @param message FUSE error message */
        explicit Exception(const std::string& message) :
            Andromeda::Utilities::Exception("FUSE Error: "+message) {}; };

    /** FUSE wrapper options */
    struct Options
    {
        /** The FUSE directory to mount */
        std::string mountPath;

        /** List of FUSE library options */
        std::list<std::string> fuseArgs;

        /** Whether fake chmod (no-op) is allowed */
        bool fakeChmod { true };

        /** Whether fake chown (no-op) is allowed */
        bool fakeChown { true };
    };

    /**
     * Starts and mounts libfuse
     * @param root andromeda folder as root
     * @param options command line options
     * @param daemonize if true, fuse_daemonize
     */
    FuseAdapter(Andromeda::FSItems::Folder& root, const Options& options, bool daemonize = false);

    /** Returns the root folder */
    Andromeda::FSItems::Folder& GetRootFolder(){ return mRootFolder; }

    /** Returns the FUSE options */
    const Options& GetOptions(){ return mOptions; }

    /** Print help text to stdout */
    static void ShowHelpText();

    /** Print version text to stdout */
    static void ShowVersionText();

private:

    Andromeda::FSItems::Folder& mRootFolder;
    const Options& mOptions;
};

} // namespace AndromedaFuse

#endif // A2FUSE_FUSEADAPTER_H


#ifndef FUSEADAPTER_H_
#define FUSEADAPTER_H_

#include <string>
#include <list>

#include "Utilities.hpp"

class Folder;

/** Static class for FUSE operations */
class FuseAdapter
{
public:

    /** Base Exception for all FUSE issues */
    class Exception : public Utilities::Exception { public:
        /** @param message FUSE error message */
        Exception(const std::string& message) :
            Utilities::Exception("FUSE Error: "+message) {}; };

    /** FUSE wrapper options */
    struct Options
    {
        /** The FUSE directory to mount */
        std::string mountPath;

        /** List of FUSE library options */
        std::list<std::string> fuseArgs;

        /** Whether fake chmod (no-op) is allowed */
        bool fakeChmod = true;

        /** Whether fake chown (no-op) is allowed */
        bool fakeChown = true;
    };

    /**
     * Starts and mounts libfuse
     * @param root andromeda folder as root
     * @param options command line options
     * @param daemonize if true, fuse_daemonize
     */
    FuseAdapter(Folder& root, const Options& options, bool daemonize);

    /** Returns the root folder */
    Folder& GetRootFolder(){ return rootFolder; }

    /** Returns the FUSE options */
    const Options& GetOptions(){ return options; }

    /** Print help text to stdout */
    static void ShowHelpText();

    /** Print version text to stdout */
    static void ShowVersionText();

private:

    Folder& rootFolder;
    const Options& options;
};

#endif // FUSEADAPTER_H
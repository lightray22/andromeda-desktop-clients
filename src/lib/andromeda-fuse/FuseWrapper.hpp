#ifndef FUSEWRAPPER_H_
#define FUSEWRAPPER_H_

#include <string>
#include <list>

#include "Utilities.hpp"

class Folder;

/** Static class for FUSE operations */
class FuseWrapper
{
public:

    /** Base Exception for all FUSE issues */
    class Exception : public Utilities::Exception { public:
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
     * Starts FUSE
     * @param root andromeda folder as root
     * @param options command line options
     */
    static void Start(Folder& root, const Options& options);

    /** Print help text to stdout */
    static void ShowHelpText();

    /** Print version text to stdout */
    static void ShowVersionText();

private:

    FuseWrapper() = delete; // static only
};

#endif
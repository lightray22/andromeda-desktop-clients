#ifndef LIBA2FUSE_FUSEOPTIONS_H_
#define LIBA2FUSE_FUSEOPTIONS_H_

#include <list>
#include <string>

namespace AndromedaFuse {

/** FUSE wrapper options */
struct FuseOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

#if !LIBFUSE2
    /** Shows the full FUSE lib help */
    static void ShowFuseHelpText();
#endif // !LIBFUSE2

    /** Adds the given argument, returning true iff it was used */
    bool AddFlag(const std::string& flag);

    /** Adds the given option/value, returning true iff it was used */
    bool AddOption(const std::string& option, const std::string& value);

    /** List of FUSE library options */
    std::list<std::string> fuseArgs;

    /** Whether fake chmod (no-op) is allowed */
    bool fakeChmod { true };

    /** Whether fake chown (no-op) is allowed */
    bool fakeChown { true };

    /** True if multi-threading is enabled */
    bool enableThreading { false }; // TODO !! enable by default once stable

#if !LIBFUSE2
    /** Maximum number of FUSE idle threads */
    unsigned int maxIdleThreads { 10 }; // FUSE's default
#endif // !LIBFUSE2
};

} // namespace Andromeda

#endif // LIBA2FUSE_FUSEOPTIONS_H_

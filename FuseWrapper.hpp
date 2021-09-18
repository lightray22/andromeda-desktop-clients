#ifndef FUSEWRAPPER_H_
#define FUSEWRAPPER_H_

#include "Utilities.hpp"

class BaseFolder;

/** Static class for FUSE operations */
class FuseWrapper
{
public:

    /**
     * Starts FUSE
     * @param root andromeda folder as root
     * @param path filesystem path to mount
     */
    static void Start(BaseFolder& root, const std::string& path);

private:

    static BaseFolder* root;

    static Debug debug;

    FuseWrapper() = delete; // static only
};

#endif
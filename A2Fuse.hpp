#ifndef A2FUSE_H_
#define A2FUSE_H_

#include "Utilities.hpp"
#include "filesystem/BaseFolder.hpp"

class A2Fuse
{
public:
    static void Start(BaseFolder& root);

private:

    static BaseFolder* root;

    static Debug debug;

    A2Fuse() = delete; // static only
};

#endif
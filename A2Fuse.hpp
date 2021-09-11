
#ifndef A2FUSE_H_
#define A2FUSE_H_

class Backend;
class BaseFolder;

class A2Fuse
{
public:
    static void Start(Backend* backend, BaseFolder* folder);

private:

    static Backend* backend;
    static BaseFolder* folder;

    A2Fuse() = delete; // static only
};

#endif
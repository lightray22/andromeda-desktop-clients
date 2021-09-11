
#ifndef A2FUSE_H_
#define A2FUSE_H_

class BaseFolder;

class A2Fuse
{
public:
    static void Start(BaseFolder* root);

private:

    static BaseFolder* root;

    A2Fuse() = delete; // static only
};

#endif
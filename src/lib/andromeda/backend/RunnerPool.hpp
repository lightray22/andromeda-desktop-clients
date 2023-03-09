#ifndef LIBA2_RUNNERPOOL_H_
#define LIBA2_RUNNERPOOL_H_

#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Backend {

class BaseRunner;

/** Manages a thread-safe pool of concurrent backend runners */
class RunnerPool
{
public:

    /** Scoped wrapper for accessing a runner under a lock */
    class LockedRunner
    {
    public:
        explicit LockedRunner(BaseRunner& runner) : mRunner(runner) { }
        BaseRunner* operator->() { return &mRunner; }
    private:
        BaseRunner& mRunner;
        // TODO !! locking
    };

    /** Initialize the pool from a single runner that will be cloned as necessary */
    explicit RunnerPool(BaseRunner& runner);

    /** Returns a locked reference to a runner */
    LockedRunner GetRunner();

private:

    BaseRunner& mRunner; // TODO !! array of runners

    Andromeda::Debug mDebug;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_RUNNERPOOL_H_

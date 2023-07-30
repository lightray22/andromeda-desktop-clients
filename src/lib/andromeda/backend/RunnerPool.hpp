#ifndef LIBA2_RUNNERPOOL_H_
#define LIBA2_RUNNERPOOL_H_

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
struct ConfigOptions;

namespace Backend {
class BaseRunner;

/** 
 * Manages a pool of concurrent backend runners 
 * THREAD SAFE (INTERNAL LOCKS)
 */
class RunnerPool
{
public:

    using UniqueLock = std::unique_lock<std::mutex>;

    /** Scoped wrapper for accessing a runner under a lock */
    class LockedRunner
    {
    public:
        explicit LockedRunner(RunnerPool& pool, BaseRunner& runner, UniqueLock lock) : 
            mPool(pool), mRunner(runner), mLock(std::move(lock)) { }

        ~LockedRunner();
        DELETE_COPY(LockedRunner)
        DELETE_MOVE(LockedRunner)

        BaseRunner& operator*() { return mRunner; }
        BaseRunner* operator->() { return &mRunner; }
    private:
        RunnerPool& mPool;
        BaseRunner& mRunner;
        UniqueLock mLock;
    };

    /** 
     * Initialize the pool from a single runner that will be cloned as necessary
     * @param options ConfigOptions containing the max pool size
     */
    explicit RunnerPool(BaseRunner& runner, const Andromeda::ConfigOptions& options);

    ~RunnerPool() = default;
    DELETE_COPY(RunnerPool)
    DELETE_MOVE(RunnerPool)

    /** Returns a reference to a runner and accompanying lock */
    LockedRunner GetRunner();

    /** Returns a const reference to the first runner */
    [[nodiscard]] const BaseRunner& GetFirst() const;

private:

    /** Signal waiting threads */
    void SignalWaiters();

    /** Array of possibly-null pointers to runners to use */
    std::vector<BaseRunner*> mRunnerPool;
    /** Array of locks for each runner */
    std::vector<std::mutex> mRunnerLocks;
    /** List of runners that we created and own */
    std::list<std::unique_ptr<BaseRunner>> mRunnersOwned;
    /** Mutex to protect the runner vectors */
    std::mutex mMutex;
    /** Condition variable to wait for a runner */
    std::condition_variable mCV;

    mutable Andromeda::Debug mDebug;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_RUNNERPOOL_H_

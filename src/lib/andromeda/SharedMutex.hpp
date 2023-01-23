
#ifndef LIBA2_SHAREDMUTEX_H_
#define LIBA2_SHAREDMUTEX_H_

#include <mutex>
#include <shared_mutex>
#include "Semaphor.hpp"

namespace Andromeda {

/** 
 * A shared mutex solving the R/W lock problem, satisfies SharedMutex
 * This class specifically implements both a readers-priority and fair queued lock,
 * unlike std::shared_mutex which does not define the priority type (up to the OS)
 * See https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem#First_readers%E2%80%93writers_problem
 */
class SharedMutex
{
public:
    inline void lock() noexcept
    {
        SemaphorLock qlock(mResQueue);
        mResource.lock();
    }

    inline void unlock() noexcept
    {
        mResource.unlock();
    }

    inline void lock_shared() noexcept
    {
        SemaphorLock qlock(mResQueue);
        std::lock_guard<std::mutex> llock(mMutex);
        if (++mReaders == 1) mResource.lock();
    }

    inline void unlock_shared() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        if (--mReaders == 0) mResource.unlock();
    }

    inline void lock_shared_priority() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        if (++mReaders == 1) mResource.lock();
    }

    inline void unlock_shared_priority() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        if (--mReaders == 0) mResource.unlock();
    }

private:
    /** current count of readers */
    size_t mReaders { 0 };
    /** mutex to guard mReaders count */
    std::mutex mMutex;
    /** mutex that is always grabbed for writing, but only grabbed for the first reader
     * need to use a Semaphor here instead of std::mutex since the unlock() can happen
     * on a different thread than the lock() which is not allowed with std::mutex */
    Semaphor mResource;
    /** preserve ordering of requests - FIFO Semaphor */
    Semaphor mResQueue;
};

typedef std::unique_lock<SharedMutex> SharedLockW;
typedef std::shared_lock<SharedMutex> SharedLockR;

/** Scope-managed shared read-priority lock */
class SharedLockRP
{
public:
    inline SharedLockRP(SharedMutex& mutex) : 
        mMutex(mutex){ mMutex.lock_shared_priority(); }
    inline ~SharedLockRP() { mMutex.unlock_shared_priority(); }

    // disallow copying
    SharedLockRP(const SharedLockRP&) = delete;
    SharedLockRP& operator=(const SharedLockRP&) = delete;
private:
    SharedMutex& mMutex;
};

} // namespace Andromeda

#endif // LIBA2_SHAREDMUTEX_H_

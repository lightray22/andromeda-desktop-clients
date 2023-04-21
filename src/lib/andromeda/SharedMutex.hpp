
#ifndef LIBA2_SHAREDMUTEX_H_
#define LIBA2_SHAREDMUTEX_H_

#include <mutex>
#include <shared_mutex>
#include <utility> // pair
#include "Semaphor.hpp"

namespace Andromeda {

/** 
 * A shared mutex solving the R/W lock problem, satisfies SharedMutex
 * This class specifically implements both a readers-priority and fair queued lock,
 * unlike std::shared_mutex which does not define the priority type (up to the OS)
 * See https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem#First_readers%E2%80%93writers_problem
 * 
 * NOTE this implementation has a small quirk - when the queue is all writers and a priority read lock
 * is acquired, it will skip the queue but only up to 2nd place in line.  E.g. (W)WWWP -> (W)WPWW ...
 * this is because only 2nd place and up are waiting on mResQueue - first place is already waiting on mResource
 */
class SharedMutex
{
public:
    inline bool try_lock() noexcept
    {
        SemaphorLock qLock(mResQueue,std::try_to_lock);
        if (!qLock) return false;
        return mResource.try_lock();
    }

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

/** Scope-managed shared lock of any type */
class SharedLock
{ 
protected:
    explicit inline SharedLock(SharedMutex& mutex, bool locked = false) :
        mMutex(mutex), mLocked(locked){ }
    
    SharedMutex& mMutex;
    bool mLocked;

    SharedLock(const SharedLock&) = delete; // no copying
    SharedLock& operator=(const SharedLock&) = delete;
    SharedLock& operator=(SharedLock&&) = delete;
};

/** Scope-managed shared read lock */
class SharedLockR : public SharedLock
{
public:
    explicit inline SharedLockR(SharedMutex& mutex) : 
        SharedLock(mutex){ lock(); }
    inline ~SharedLockR(){ unlock(); }

    inline void lock()
    {
        if (mLocked) return;
        mMutex.lock_shared();
        mLocked = true;
    }

    inline void unlock()
    { 
        if (!mLocked) return;
        mMutex.unlock_shared();
        mLocked = false; 
    }

    explicit inline SharedLockR(SharedLockR&& lock) : // move
        SharedLock(lock.mMutex, lock.mLocked){ 
            lock.mLocked = false; }
};

/** Scope-managed shared read-priority lock */
class SharedLockRP : public SharedLock
{
public:
    explicit inline SharedLockRP(SharedMutex& mutex) : 
        SharedLock(mutex){ lock(); }
    inline ~SharedLockRP(){ unlock(); }

    inline void lock()
    {
        if (mLocked) return;
        mMutex.lock_shared_priority();
        mLocked = true;
    }

    inline void unlock()
    { 
        if (!mLocked) return;
        mMutex.unlock_shared_priority(); 
        mLocked = false;
    }
    
    explicit inline SharedLockRP(SharedLockRP&& lock) : // move
         SharedLock(lock.mMutex, lock.mLocked){ 
            lock.mLocked = false; }
};

/** Scope-managed shared write lock */
class SharedLockW : public SharedLock
{
public:
    explicit inline SharedLockW(SharedMutex& mutex) : 
        SharedLock(mutex){ lock(); }
    inline ~SharedLockW(){ unlock(); }

    inline void lock()
    {
        if (mLocked) return;
        mMutex.lock();
        mLocked = true;
    }

    inline void unlock()
    { 
        if (!mLocked) return;
        mMutex.unlock();
        mLocked = false;
    }

    explicit inline SharedLockW(SharedLockW&& lock) : // move
         SharedLock(lock.mMutex, lock.mLocked){ 
            lock.mLocked = false; }

    /** Get a SharedLockW for each of two locks, deadlock-safe */
    typedef std::pair<SharedLockW, SharedLockW> LockPair;
    static LockPair get_pair(SharedMutex& mutex1, SharedMutex& mutex2)
    {
        std::lock(mutex1, mutex2); // no deadlock
        return std::make_pair(
            SharedLockW(mutex1, true), 
            SharedLockW(mutex2, true));
    }

protected:
    explicit inline SharedLockW(SharedMutex& mutex, bool locked) : 
        SharedLock(mutex, locked){ }
};

} // namespace Andromeda

#endif // LIBA2_SHAREDMUTEX_H_

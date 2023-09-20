
#ifndef LIBA2_SHAREDMUTEX_H_
#define LIBA2_SHAREDMUTEX_H_

#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility> // pair

namespace Andromeda {

/**
 * A shared mutex solving the R/W lock problem, satisfies SharedMutex
 * This class specifically implements both a readers-priority and fair queued lock,
 * unlike std::shared_mutex which does not define the priority type (up to the OS)
 */
class SharedMutex
{
public:
    inline bool try_lock() noexcept
    {
        const std::lock_guard<std::mutex> llock(mMutex);

        if (!mLocked && mQueue.empty())
        {
            mLocked = true;
            return true;
        }
        else return false; 
    }

    inline void lock() noexcept
    {
        std::unique_lock<std::mutex> llock(mMutex);

        const size_t waitIdx { mIndex++ };
        mQueue.emplace_back(waitIdx);

        while (mLocked || mQueue.front() != waitIdx)
            mWaitCV.wait(llock);

        mQueue.pop_front();
        mLocked = true;
    }

    inline void unlock() noexcept
    {
        const std::lock_guard<std::mutex> llock(mMutex);

        mLocked = false;
        if (!mQueue.empty())
            mWaitCV.notify_all();
    }

    /** @param priority if true, skip to the front of the queue */
    inline void lock_shared(bool priority = false) noexcept
    {
        std::unique_lock<std::mutex> llock(mMutex);

        if (priority && mReaders){
            ++mReaders; return; }

        const size_t waitIdx { mIndex++ };
        if (priority)
            mQueue.emplace_front(waitIdx);
        else mQueue.emplace_back(waitIdx);

        while ((!mReaders && mLocked) || mQueue.front() != waitIdx)
            mWaitCV.wait(llock);

        mQueue.pop_front();
        if (!(mReaders++)) // 0->1
            mLocked = true;

        if (!mQueue.empty())
            mWaitCV.notify_all();
    }

    inline void unlock_shared() noexcept
    {
        const std::lock_guard<std::mutex> llock(mMutex);

        if (!(--mReaders)) // 1->0
            mLocked = false;
        if (!mQueue.empty())
            mWaitCV.notify_all();
    }

private:

    /** Mutex to protect member vars */
    std::mutex mMutex;
    /** CV used to sleep/wake waiters */
    std::condition_variable mWaitCV;
    /** Queue used to order locks */
    std::deque<size_t> mQueue;
    /** Next thread ID to use */
    size_t mIndex { 0 };

    /** Current number readers */
    size_t mReaders { 0 };
    /** True when the mutex is held */
    bool mLocked { false };
};

/** Scope-managed shared lock of any type */
class SharedLock { };

/** Scope-managed shared read lock */
class SharedLockR : public SharedLock
{
public:
    explicit inline SharedLockR(SharedMutex& mutex) : 
        mMutex(mutex) { lock(); }
    
    inline ~SharedLockR(){ if (mLocked) unlock(); }

    inline void lock()
    {
        mMutex.lock_shared();
        mLocked = true;
    }

    inline void unlock()
    { 
        mLocked = false; 
        mMutex.unlock_shared();
    }

    inline explicit operator bool() const { return mLocked; }

    inline SharedLockR(SharedLockR&& lock) noexcept : // move
        mMutex(lock.mMutex), mLocked(lock.mLocked){ lock.mLocked = false; }
    
    inline SharedLockR(const SharedLockR&) = delete; // no copy
    inline SharedLockR& operator=(const SharedLockR&) = delete;
    inline SharedLockR& operator=(SharedLockR&&) = delete;
    
private:
    SharedMutex& mMutex;
    bool mLocked { false };
};

/** Scope-managed shared read-priority lock */
class SharedLockRP : public SharedLock
{
public:
    explicit inline SharedLockRP(SharedMutex& mutex) : 
        mMutex(mutex) { lock(); }
    
    inline ~SharedLockRP(){ if (mLocked) unlock(); }

    inline void lock()
    {
        mMutex.lock_shared(true);
        mLocked = true;
    }

    inline void unlock()
    { 
        mLocked = false;
        mMutex.unlock_shared(); 
    }
    
    inline explicit operator bool() const { return mLocked; }

    inline SharedLockRP(SharedLockRP&& lock) noexcept : // move
        mMutex(lock.mMutex), mLocked(lock.mLocked){ lock.mLocked = false; }

    inline SharedLockRP(const SharedLockRP&) = delete; // no copy
    inline SharedLockRP& operator=(const SharedLockRP&) = delete;
    inline SharedLockRP& operator=(SharedLockRP&&) = delete;
    
private:
    SharedMutex& mMutex;
    bool mLocked { false };
};

/** Scope-managed exclusive write lock */
class SharedLockW : public SharedLock
{
public:
    explicit inline SharedLockW(SharedMutex& mutex) : 
        mMutex(mutex) { lock(); }
    explicit inline SharedLockW(SharedMutex& mutex, std::try_to_lock_t dt) : 
        mMutex(mutex) { try_lock(); }
    
    inline ~SharedLockW(){ if (mLocked) unlock(); }

    inline bool try_lock()
    {
        if (mLocked) return false;
        mLocked = mMutex.try_lock();
        return mLocked;
    }

    inline void lock()
    {
        mMutex.lock();
        mLocked = true;
    }

    inline void unlock()
    { 
        mLocked = false;
        mMutex.unlock();
    }

    inline explicit operator bool() const { return mLocked; }

    inline SharedLockW(SharedLockW&& lock) noexcept : // move
        mMutex(lock.mMutex), mLocked(lock.mLocked){ lock.mLocked = false; }

    inline SharedLockW(const SharedLockW&) = delete; // no copy
    inline SharedLockW& operator=(const SharedLockW&) = delete;
    inline SharedLockW& operator=(SharedLockW&&) = delete;
    
    /** Get a SharedLockW for each of two locks, deadlock-safe */
    using LockPair = std::pair<SharedLockW, SharedLockW>;
    static LockPair get_pair(SharedMutex& mutex1, SharedMutex& mutex2)
    {
        std::lock(mutex1, mutex2); // deadlock-safe - try_lock
        return std::make_pair(
            SharedLockW(mutex1, true), 
            SharedLockW(mutex2, true));
    }

protected:
    explicit inline SharedLockW(SharedMutex& mutex, bool locked) : // for get_pair
        mMutex(mutex), mLocked(locked) { }

private:
    SharedMutex& mMutex;
    bool mLocked { false };
};

} // namespace Andromeda

#endif // LIBA2_SHAREDMUTEX_H_

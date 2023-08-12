
#ifndef LIBA2_SEMAPHOR_H_
#define LIBA2_SEMAPHOR_H_

#include <mutex>
#include <condition_variable>

namespace Andromeda {

/** 
 * Simple counting FIFO (queued) semaphor - not recursive
 * Satisfies Mutex - works with lock_guard!
 */
class Semaphor
{
public:

    /** @param max maximum number of concurrent lock holders */
    explicit Semaphor(size_t max = 1) :
        mMaxCount(max), mAvailable(max), mCurSignal(max) { }

    /** Locks the semaphor and returns true if there are NO waiters (caller is first in queue) */
    inline bool try_lock() noexcept
    {
        const std::lock_guard<std::mutex> llock(mMutex);

        const size_t waitIndex { ++mCurWait };
        if (must_wait(waitIndex))
        {
            --mCurWait; // restore
            return false;
        }
        if (--mAvailable)
            mWaitCV.notify_all();
        return true;
    }

    /** Locks the semaphor (in queue order!) */
    inline void lock() noexcept
    {
        std::unique_lock<std::mutex> llock(mMutex);

        const size_t waitIndex { ++mCurWait };
        while (must_wait(waitIndex))
            mWaitCV.wait(llock);
        if (--mAvailable)
            mWaitCV.notify_all();
    }

    /** Unlocks the semaphor, signals waiters */
    inline void unlock() noexcept
    {
        const std::lock_guard<std::mutex> llock(mMutex);

        ++mAvailable;
        ++mCurSignal;
        mWaitCV.notify_all();
    }

    /** Returns the max semaphor count */
    inline size_t get_max() noexcept
    {
        const std::lock_guard<std::mutex> llock(mMutex);
        return mMaxCount;
    }

    /** Changes the max semaphor count (blocking) */
    inline void set_max(size_t max) noexcept
    {
        std::unique_lock<std::mutex> llock(mMutex);

        if (max > mMaxCount)
        {
            // unlock N times
            mAvailable += max-mMaxCount;
            mCurSignal += max-mMaxCount;
            mMaxCount = max;
            mWaitCV.notify_all();
        }
        else while (max < mMaxCount)
        {
            // Un-unlock N times, waiting for each
            while (!mAvailable)
                mWaitCV.wait(llock);
            --mAvailable;
            --mCurSignal;
            --mMaxCount;
        }
    }

private:

    // overflow-safe returns true if this waitIndex must wait - MUST BE LOCKED
    [[nodiscard]] inline bool must_wait(const size_t waitIndex) const
    {
        return !mAvailable || waitIndex + mAvailable-1 != mCurSignal;
    }

    /** original max count */
    size_t mMaxCount;
    /** current available locks */
    size_t mAvailable;
    /** next index of waiter - overflow safe */
    size_t mCurWait { 0 };
    /** index of current signal - overflow safe */
    size_t mCurSignal;

    std::mutex mMutex;
    std::condition_variable mWaitCV;
};

using SemaphorLock = std::unique_lock<Semaphor>;

} // namespace Andromeda

#endif // LIBA2_SEMAPHOR_H_

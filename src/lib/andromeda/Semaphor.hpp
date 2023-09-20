
#ifndef LIBA2_SEMAPHOR_H_
#define LIBA2_SEMAPHOR_H_

#include <condition_variable>
#include <deque>
#include <mutex>

namespace Andromeda {

/** 
 * Simple counting FIFO (queued) semaphor - not recursive
 * Satisfies Mutex - works with lock_guard!
 */
class Semaphor
{
public:
    /** @param max maximum number of concurrent lock holders */
    explicit Semaphor(size_t max = 1):
        mAvailable(max), mMaxCount(max) { }

    /** Locks the semaphor and returns true if there are NO waiters (caller is first in queue) */
    inline bool try_lock() noexcept
    {
        const std::lock_guard<std::mutex> llock(mMutex);

        if (mAvailable && mQueue.empty())
        {
            --mAvailable;
            return true;
        }
        else return false;
    }

    /** Locks the semaphor (in queue order!) */
    inline void lock() noexcept
    {
        std::unique_lock<std::mutex> llock(mMutex);

        const size_t waitIdx { mIndex++ };
        mQueue.emplace_back(waitIdx);

        while (!mAvailable || mQueue.front() != waitIdx)
            mWaitCV.wait(llock);

        mQueue.pop_front();
        --mAvailable;

        if (mAvailable && !mQueue.empty())
            mWaitCV.notify_all();
    }

    /** Unlocks the semaphor, signals waiters */
    inline void unlock() noexcept
    {
        const std::lock_guard<std::mutex> llock(mMutex);

        ++mAvailable;
        if (!mQueue.empty())
            mWaitCV.notify_all();
    }

    /** Returns the max semaphor count */
    inline size_t get_max() noexcept
    {
        const std::lock_guard<std::mutex> llock(mMutex);
        return mMaxCount;
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

    /** Current number of locks free */
    size_t mAvailable;
    /** Max count of the semaphor */
    size_t mMaxCount;
};

using SemaphorLock = std::unique_lock<Semaphor>;

} // namespace Andromeda

#endif // LIBA2_SEMAPHOR_H_

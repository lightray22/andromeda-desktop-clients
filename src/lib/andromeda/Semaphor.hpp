
#ifndef LIBA2_SEMAPHOR_H_
#define LIBA2_SEMAPHOR_H_

#include <mutex>
#include <condition_variable>

namespace Andromeda {

/** 
 * Simple counting FIFO semaphor - not recursive
 * Satisfies Mutex - works with lock_guard!
 */
class Semaphor
{
public:
    /** @param max maximum number of concurrent lock holders */
    Semaphor(size_t max = 1) : mMaxCount(max) { }

    inline bool try_lock() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        if (mCount == mMaxCount) return false;
        ++mCount; return true;
    }

    inline void lock() noexcept
    {
        std::unique_lock<std::mutex> llock(mMutex);
        if (mCount == mMaxCount)
        {
            const size_t waitIndex { ++curWait };
            while (waitIndex != curSignal)
                mCV.wait(llock);
        }
        ++mCount;
    }

    inline void unlock() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        if (curWait != curSignal)
        {
            ++curSignal;
            mCV.notify_all();
        }
        --mCount;
    }

    /** Returns the current # of locks */
    size_t get_count() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        return mCount;
    }

    /** Returns the max semaphor count */
    size_t get_max() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        return mMaxCount;
    }

private:

    /** current count */
    size_t mCount { 0 };
    /** original max count */
    size_t mMaxCount;
    /** index of current waiter - can overflow */
    size_t curWait { 0 };
    /** index of current signal - can overflow */
    size_t curSignal { 0 };

    std::mutex mMutex;
    std::condition_variable mCV;
};

typedef std::unique_lock<Semaphor> SemaphorLock;

} // namespace Andromeda

#endif // LIBA2_SEMAPHOR_H_

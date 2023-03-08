
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
    Semaphor(size_t max = 1, bool debug = false) :
        mAvailable(max), mMaxCount(max), mCurSignal(max+1) { }

    inline bool try_lock() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        if (!mAvailable) return false;
        --mAvailable; return true;
    }

    inline void lock() noexcept
    {
        std::unique_lock<std::mutex> llock(mMutex);
        
        const size_t waitIndex { ++mCurWait };
        while (!mAvailable || waitIndex + mAvailable != mCurSignal)
            mCV.wait(llock);
        --mAvailable;
    }

    inline void unlock() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);

        ++mCurSignal;
        ++mAvailable;
        mCV.notify_all();
    }

    /** Returns the current # of locks */
    size_t get_avail() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        return mAvailable;
    }

    /** Returns the max semaphor count */
    size_t get_max() noexcept
    {
        std::lock_guard<std::mutex> llock(mMutex);
        return mMaxCount;
    }

private:

    /** current available locks */
    size_t mAvailable;
    /** original max count */
    size_t mMaxCount;
    /** index of current waiter - can overflow */
    size_t mCurWait { 0 };
    /** index of current signal - can overflow */
    size_t mCurSignal;

    std::mutex mMutex;
    std::condition_variable mCV;
};

typedef std::unique_lock<Semaphor> SemaphorLock;

} // namespace Andromeda

#endif // LIBA2_SEMAPHOR_H_

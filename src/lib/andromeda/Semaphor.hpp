
#ifndef LIBA2_SEMAPHOR_H_
#define LIBA2_SEMAPHOR_H_

#include <mutex>
#include <condition_variable>

namespace Andromeda {

/** 
 * Simple counting semaphor - not recursive
 * Satisfies Mutex - works with lock_guard!
 */
class Semaphor
{
public:

    Semaphor(size_t max = 1) : mMaxCount(max) { }

    bool try_lock()
    {
        std::lock_guard<std::mutex> llock(mMutex);
        if (mCount+1 < mMaxCount) { ++mCount; return true; } 
        else return false;
    }

    void lock()
    {
        std::unique_lock<std::mutex> llock(mMutex);
        while (mCount+1 >= mMaxCount) mCV.wait(llock); 
        ++mCount;
    }

    void unlock()
    {
        std::lock_guard<std::mutex> llock(mMutex);
        --mCount; mCV.notify_one();
    }

    /** Returns the current # of locks */
    size_t get_count()
    {
        std::lock_guard<std::mutex> llock(mMutex);
        return mCount;
    }

    /** Returns the max semaphor count */
    size_t get_max()
    {
        std::lock_guard<std::mutex> llock(mMutex);
        return mMaxCount;
    }

    /** Change the semaphor max count */
    void set_max(size_t max)
    {
        std::unique_lock<std::mutex> llock(mMutex);

        while (max > mMaxCount)
        {
            ++mMaxCount;
            mCV.notify_one();
        }
        while (max < mMaxCount)
        {
            while (mCount+1 >= mMaxCount) mCV.wait(llock); 
            --mMaxCount;
        }

        mMaxCount = max;
    }

private:
    /** current count */
    size_t mCount { 0 };
    /** original max count */
    size_t mMaxCount;

    std::mutex mMutex;
    std::condition_variable mCV;
};

typedef std::unique_lock<Semaphor> SemaphorLock;

} // namespace Andromeda

#endif // LIBA2_SEMAPHOR_H_

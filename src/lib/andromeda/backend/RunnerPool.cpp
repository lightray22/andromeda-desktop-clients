
#include "BaseRunner.hpp"
#include "RunnerPool.hpp"
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;

namespace Andromeda {
namespace Backend {

/*****************************************************/
RunnerPool::RunnerPool(BaseRunner& runner, const ConfigOptions& options) :
    mRunnerPool(options.runnerPoolSize, nullptr),
    mRunnerLocks(options.runnerPoolSize),
    mDebug(__func__,this)
{
    MDBG_INFO("(poolSize:" << mRunnerPool.size() << ")");
    mRunnerPool[0] = &runner; // first is never null
}

/*****************************************************/
const BaseRunner& RunnerPool::GetFirst() const { return *mRunnerPool[0]; }

/*****************************************************/
RunnerPool::LockedRunner RunnerPool::GetRunner()
{
    UniqueLock llock(mMutex);
    MDBG_INFO("()");

    size_t idx { 0 }; while (true)
    {
        UniqueLock rlock(mRunnerLocks[idx], std::try_to_lock);
        if (!rlock) // not locked
        {
            if (idx+1 == mRunnerPool.size()) // all busy, wait
            {
                MDBG_INFO("... waiting!");
                idx = 0; // start over
                mCV.wait(llock); 
            }
            else ++idx; // try next
        }
        else // have lock
        {
            if (!mRunnerPool[idx]) // not initialized
            {
                MDBG_INFO("... new runner:" << idx);
                mRunnersOwned.emplace_back(mRunnerPool[0]->Clone());
                mRunnerPool[idx] = mRunnersOwned.back().get();
            }

            MDBG_INFO("... return runner:" << idx);
            return LockedRunner(*this, *mRunnerPool[idx], std::move(rlock));
        }
    }
}

/*****************************************************/
void RunnerPool::SignalWaiters()
{
    UniqueLock llock(mMutex);
    MDBG_INFO("()");
    mCV.notify_one();
}

/*****************************************************/
RunnerPool::LockedRunner::~LockedRunner()
{
    mLock.unlock();
    mPool.SignalWaiters();
}

} // namespace Backend
} // namespace Andromeda

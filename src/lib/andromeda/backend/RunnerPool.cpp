
#include "BaseRunner.hpp"
#include "RunnerPool.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
RunnerPool::RunnerPool(BaseRunner& runner) : 
    mRunner(runner), mDebug(__func__,this)
{

}

/*****************************************************/
RunnerPool::LockedRunner RunnerPool::GetRunner()
{
    return LockedRunner(mRunner);
}

} // namespace Backend
} // namespace Andromeda

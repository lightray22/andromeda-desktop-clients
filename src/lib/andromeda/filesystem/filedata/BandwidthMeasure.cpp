
#include <numeric>

#include "BandwidthMeasure.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
BandwidthMeasure::BandwidthMeasure(const char* debugName, const std::chrono::milliseconds& timeTarget):
    mTimeTarget(timeTarget), mDebug(std::string(__func__)+"_"+debugName,this) { }

/*****************************************************/
size_t BandwidthMeasure::UpdateBandwidth(const size_t bytes, const std::chrono::steady_clock::duration& time)
{
    using namespace std::chrono;

    MDBG_INFO("(bytes:" << bytes << " time(ms):" << duration_cast<milliseconds>(time).count());

    if (bytes > 0) // useless if 0
    {
        MDBG_INFO("... bandwidth:" << (static_cast<double>(bytes)/duration<double>(time).count()/(1<<20)) << " MiB/s");

        const double timeFrac { duration<double>(time) / duration<double>(mTimeTarget) };
        const size_t targetBytesN { static_cast<size_t>(static_cast<double>(bytes) / timeFrac) };
        MDBG_INFO("... timeFrac:" << timeFrac << " targetBytes:" << targetBytesN);

        mBandwidthHistory[mBandwidthHistoryIdx] = targetBytesN;
        mBandwidthHistoryIdx = (mBandwidthHistoryIdx+1) % BANDWIDTH_WINDOW;
    }
    
    const size_t targetBytes { std::accumulate(mBandwidthHistory.cbegin(), mBandwidthHistory.cend(), static_cast<size_t>(0)) / BANDWIDTH_WINDOW };
    MDBG_INFO("... return targetBytes:" << targetBytes); return targetBytes;
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

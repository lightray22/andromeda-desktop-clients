
#ifndef LIBA2_BANDWIDTHMEASURE_H_
#define LIBA2_BANDWIDTHMEASURE_H_

#include <array>
#include <chrono>

#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/** 
 * Keeps a history of bandwidth measurements to
 * calculate the ideal size for network transfers
 */
class BandwidthMeasure
{
public:

    explicit BandwidthMeasure(Debug& debug): mDebug(debug) { };

    virtual ~BandwidthMeasure(){ };

    /** Updates the bandwidth history with the given measure and returns the estimated targetBytes */
    uint64_t UpdateBandwidth(const size_t bytes, const std::chrono::steady_clock::duration& time);

private:

    /** The desired time target for the transfer */
    const std::chrono::milliseconds mTimeTarget { 1000 };
    
    /** The number of bandwidth history entries to store */
    static constexpr size_t BANDWIDTH_WINDOW { 4 };

    /** Array of calculated bandwidth targetBytes to average together */
    std::array<uint64_t, BANDWIDTH_WINDOW> mBandwidthHistory { };

    /** The next index of mBandwidthHistory to write with a measurement */
    size_t mBandwidthHistoryIdx { 0 };

    Debug& mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_BANDWIDTHMEASURE_H_

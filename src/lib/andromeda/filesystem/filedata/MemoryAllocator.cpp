
#include <cassert>
#include <memory>
#include <ostream>

#if WIN32
#include <windows.h>
#else // !WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif // WIN32

#include "MemoryAllocator.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
MemoryAllocator::MemoryAllocator() : mDebug(__func__,this){ }

#if DEBUG
MemoryAllocator::~MemoryAllocator(){ assert(mAllocMap.empty()); }
#endif // DEBUG

/*****************************************************/
size_t MemoryAllocator::get_usage(size_t bytes)
{
    if (!bytes) return 0;

#if WITHOUT_MMAP
    size_t pageSize = 1;
#elif WIN32
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    DWORD pageSize = systemInfo.dwAllocationGranularity;
#else // MMAP && !WIN32
    size_t pageSize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif // MMAP/WIN32

    const size_t pages = (bytes-1)/pageSize+1;
    return pages*pageSize;
}

/*****************************************************/
void* MemoryAllocator::alloc(size_t bytes)
{
    if (!bytes) return nullptr;

#ifdef WITHOUT_MMAP
    void* ptr = std::malloc(bytes);
#elif WIN32
    LPVOID ptr = VirtualAlloc(NULL, bytes, 
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else // MMAP && !WIN32
    void* ptr = mmap(NULL, bytes, 
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif // MMAP/WIN32
    MDBG_INFO("(ptr:" << ptr << " bytes:" << bytes << ")");

#if DEBUG
{ // lock scope
    const LockGuard lock(mMutex);
    mAllocMap.emplace(ptr, bytes);
}
#endif // DEBUG

    static const std::string fname { __func__ };
    stats(fname, bytes, true);
    return ptr;
}

/*****************************************************/
void MemoryAllocator::free(void* const ptr, const size_t bytes)
{
    MDBG_INFO("(ptr:" << ptr << " bytes:" << bytes << ")");
    if (ptr == nullptr) return;

#if DEBUG
{ // lock scope
    const LockGuard lock(mMutex);
    AllocMap::iterator it { mAllocMap.find(ptr) };
    assert(it != mAllocMap.end());
    assert(it->second == bytes);
    mAllocMap.erase(it);
}
#endif // DEBUG

#ifdef WITHOUT_MMAP
    std::free(ptr);
#elif WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else // MMAP && !WIN32
    munmap(ptr, bytes);
#endif // MMAP/WIN32

    static const std::string fname { __func__ };
    stats(fname, bytes, false);
}

/*****************************************************/
void MemoryAllocator::stats(const std::string& fname, const size_t bytes, bool alloc) 
{
    mDebug.Info([&](std::ostream& str)
    {
        const LockGuard lock(mMutex);

        if (alloc) { ++mAllocs; mTotal += bytes; }
        else { ++mFrees; mTotal -= bytes; }

        str << fname << "... mTotal:" << mTotal 
    #if DEBUG
        << " mAllocMap:" << mAllocMap.size()
    #endif // DEBUG
        << " mAllocs:" << mAllocs << " mFrees:" << mFrees; 
    });
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

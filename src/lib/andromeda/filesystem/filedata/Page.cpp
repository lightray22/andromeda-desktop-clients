

#include <cstring>

#include "CachingAllocator.hpp"
#include "Page.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
Page::Page(size_t pageSize, CachingAllocator& memAlloc) : mAlloc(memAlloc)
{
    mBytes = pageSize;
    mPages = mAlloc.getNumPages(mBytes);
    mData = mPages ? reinterpret_cast<char*>(mAlloc.alloc(mPages)) : nullptr;
}

/*****************************************************/
Page::Page(Page&& page) : mAlloc(page.mAlloc) // move constructor
{
    mBytes = page.mBytes;
    mPages = page.mPages;
    mData = page.mData;

    page.mBytes = 0;
    page.mPages = 0;
    page.mData = nullptr;
}

/*****************************************************/
Page::~Page()
{
    if (mData != nullptr)
        mAlloc.free(mData, mPages);
}

/*****************************************************/
size_t Page::capacity() const
{ 
    return mAlloc.getNumBytes(mBytes); 
}

/*****************************************************/
void Page::resize(size_t bytes)
{
    const size_t pages { mAlloc.getNumPages(bytes) };
    if (pages != mPages) // re-allocate
    {
        char* const data { reinterpret_cast<char*>(mAlloc.alloc(pages)) };
        if (mData != nullptr)
        {
            std::memcpy(data, mData, std::min(bytes,mBytes));
            mAlloc.free(mData, mPages);
        }

        mBytes = bytes;
        mPages = pages;
        mData = data;
    }
    else mBytes = bytes;
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
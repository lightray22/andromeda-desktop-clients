
set(SOURCE_FILES 
    BandwidthMeasure.cpp
    CacheManager.cpp
    CacheOptions.cpp
    CachingAllocator.cpp
    MemoryAllocator.cpp
    Page.cpp
    PageBackend.cpp
    PageManager.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

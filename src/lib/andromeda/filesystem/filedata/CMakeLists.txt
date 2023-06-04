
set(SOURCE_FILES 
    BandwidthMeasure.cpp
    CacheManager.cpp
    CacheOptions.cpp
    CachingAllocator.cpp
    MemoryAllocator.cpp
    PageBackend.cpp
    PageManager.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

option(WITHOUT_MMAP "Don't use manual mmap for page allocations" OFF)
if (${WITHOUT_MMAP})
    target_compile_definitions(libandromeda PRIVATE WITHOUT_MMAP)
endif()

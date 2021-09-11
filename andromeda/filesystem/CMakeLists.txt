
set(SOURCE_FILES 
    BaseFolder.cpp
    File.cpp
    FileData.cpp
    FileDataMgr.cpp
    Folder.cpp
    Item.cpp 
    )

target_sources(andromeda PRIVATE ${SOURCE_FILES})

add_subdirectory(folders)

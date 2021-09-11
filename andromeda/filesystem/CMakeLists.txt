
set(SOURCE_FILES 
    BaseFolder.cpp
    DataManager.cpp
    File.cpp
    FileData.cpp
    Item.cpp 
    )

target_sources(andromeda PRIVATE ${SOURCE_FILES})

add_subdirectory(folders)


set(SOURCE_FILES 
    DataManager.cpp
    File.cpp
    FileData.cpp
    Folder.cpp
    Item.cpp 
    )

target_sources(andromeda PRIVATE ${SOURCE_FILES})

add_subdirectory(folders)

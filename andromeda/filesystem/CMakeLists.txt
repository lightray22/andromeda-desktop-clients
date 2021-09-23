
set(SOURCE_FILES 
    File.cpp
    Folder.cpp
    Item.cpp 
    )

target_sources(andromeda PRIVATE ${SOURCE_FILES})

add_subdirectory(folders)


set(SOURCE_FILES 
    File.cpp
    Folder.cpp
    FSConfig.cpp
    Item.cpp 
    )

target_sources(andromeda PRIVATE ${SOURCE_FILES})

add_subdirectory(filedata)
add_subdirectory(folders)

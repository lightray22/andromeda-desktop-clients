
set(SOURCE_FILES BaseFolder.cpp
                File.cpp
                FileData.cpp
                Folder.cpp
                Item.cpp 
)

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

add_subdirectory(folders)

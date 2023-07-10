
set(SOURCE_FILES 
    MixedInput.cpp
    MixedOutput.cpp
    ObjectDatabase.cpp
    SqliteDatabase.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

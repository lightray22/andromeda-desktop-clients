
set(SOURCE_FILES 
    BaseObject.cpp
    FieldTypes.cpp
    MixedInput.cpp
    MixedOutput.cpp
    ObjectDatabase.cpp
    QueryBuilder.cpp
    SqliteDatabase.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

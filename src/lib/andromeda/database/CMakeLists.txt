
set(SOURCE_FILES 
    BaseObject.cpp
    MixedValue.cpp
    ObjectDatabase.cpp
    QueryBuilder.cpp
    SqliteDatabase.cpp
    TableBuilder.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

add_subdirectory(fieldtypes)

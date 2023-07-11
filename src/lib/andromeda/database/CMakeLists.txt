
set(SOURCE_FILES 
    BaseObject.cpp
    FieldTypes.cpp
    MixedValue.cpp
    ObjectDatabase.cpp
    QueryBuilder.cpp
    SqliteDatabase.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

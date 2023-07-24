
set(SOURCE_FILES 
    MixedValueTest.cpp
    ObjectDatabaseTest.cpp
    QueryBuilderTest.cpp
    SqliteDatabaseTest.cpp
    TableBuilderTest.cpp
    )

target_sources(libandromeda_tests PRIVATE ${SOURCE_FILES})

add_subdirectory(fieldtypes)

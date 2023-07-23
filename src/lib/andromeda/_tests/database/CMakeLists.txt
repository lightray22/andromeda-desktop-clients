
set(SOURCE_FILES 
    FieldTypesTest.cpp
    ObjectDatabaseTest.cpp
    QueryBuilderTest.cpp
    SqliteDatabaseTest.cpp
    )

target_sources(libandromeda_tests PRIVATE ${SOURCE_FILES})

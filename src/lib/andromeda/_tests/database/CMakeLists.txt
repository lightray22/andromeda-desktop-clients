
set(SOURCE_FILES 
    ObjectDatabaseTest.cpp
    QueryBuilderTest.cpp
    SqliteDatabaseTest.cpp
    )

target_sources(libandromeda_tests PRIVATE ${SOURCE_FILES})

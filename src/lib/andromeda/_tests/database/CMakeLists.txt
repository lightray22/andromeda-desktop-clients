
set(SOURCE_FILES 
    BaseObjectTest.cpp
    FieldTypesTest.cpp
    ObjectDatabaseTest.cpp
    QueryBuilderTest.cpp
    SqliteDatabaseTest.cpp
    )

target_sources(libandromeda_tests PRIVATE ${SOURCE_FILES})

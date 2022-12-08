
set(SOURCE_FILES 
    BackendImpl.cpp
    CLIRunner.cpp
    Config.cpp
    ConfigOptions.cpp
    HTTPOptions.cpp
    HTTPRunner.cpp
    )

target_sources(andromeda PRIVATE ${SOURCE_FILES})

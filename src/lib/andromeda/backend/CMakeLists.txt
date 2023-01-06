
set(SOURCE_FILES 
    BackendImpl.cpp
    CLIRunner.cpp
    Config.cpp
    ConfigOptions.cpp
    HTTPOptions.cpp
    HTTPRunner.cpp
    RunnerInput.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

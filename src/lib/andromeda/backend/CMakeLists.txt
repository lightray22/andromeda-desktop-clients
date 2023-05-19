
set(SOURCE_FILES 
    BackendImpl.cpp
    CLIRunner.cpp
    Config.cpp
    HTTPOptions.cpp
    HTTPRunner.cpp
    RunnerInput.cpp
    RunnerOptions.cpp
    RunnerPool.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

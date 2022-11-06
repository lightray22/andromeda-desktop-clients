cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC)
#    set(ANDROMEDA_WARNINGS /W4 /WX) # TODO enable me
else()
    set(ANDROMEDA_WARNINGS -Wall -Wextra -Werror
        -Wno-unused-parameter -pedantic -pedantic-errors)
endif()

Include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)


This is a FUSE client for an Andromeda backend API.

# Building

`mkdir build; cd build; cmake ..; cmake --build .`

`apt install make cmake g++ libfuse3-dev nlohmann-json3-dev`

`cmake -DCMAKE_BUILD_TYPE=Debug`

Requires cmake >= 3.18, C++17 support (GCC 8+), libfuse >= 3.10
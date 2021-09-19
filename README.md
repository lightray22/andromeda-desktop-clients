
This is a FUSE client for an Andromeda backend API (early development).

# Prerequisites

Requires cmake >= 3.18, C++17 support (GCC 8+), libfuse (>=3.10), nlohmann-json (>=3), libssl, libcrypt

Ubuntu: `apt install make cmake g++ fuse3 libfuse3-dev nlohmann-json3-dev libssl-dev libcrypt-dev`

https://github.com/libfuse/libfuse
https://github.com/nlohmann/json

Some other dependencies are included in thirdparty/ and built here.

# Building

- Get submodules `git submodule update --init`
- Make build folder `mkdir build; cd build`
- Initialize cmake `cmake -DCMAKE_BUILD_TYPE="Debug|Release" ..`
- Run compile `cmake --build .`

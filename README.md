
This is a FUSE client for an Andromeda backend API (early development).

# Prerequisites

### Build System

- C++17 compiler (GCC 8+ or Clang 5+)
- cmake >= 3.18
- Python3

### Libraries

- libssl, libcrypto >= 1.1.1 (should be in system)
- libfuse (>=3.10)
- nlohmann-json (>=3)

Ubuntu: `apt install make cmake g++ fuse3 libfuse3-dev nlohmann-json3-dev libssl-dev libcrypt-dev`
FreeBSD: `pkg install cmake fusefs-libs3 nlohmann-json`

https://github.com/libfuse/libfuse
https://github.com/nlohmann/json

Some other dependencies are included in thirdparty/ and built here.

# Building

- Get submodules `git submodule update --init`
- Make build folder `mkdir build; cd build`
- Initialize cmake `cmake -DCMAKE_BUILD_TYPE="Debug|Release" ..`
- Run compile `cmake --build .`

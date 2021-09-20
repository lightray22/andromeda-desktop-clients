
This is a FUSE client for an Andromeda backend API (early development).

# Prerequisites

Some other dependencies are included in thirdparty/ and built in-tree.

### Build System

- C++17 compiler (GCC 8+ or Clang 5+)
- cmake (>= 3.18)
- Python3

### Libraries

- OpenSSL (>= 1.1.1) (libssl, libcrypto)
- libfuse (>= 3.10) https://github.com/libfuse/libfuse
- nlohmann-json (>= 3) https://github.com/nlohmann/json

### OS Examples

- Ubuntu: `apt install make cmake g++ fuse3 libfuse3-dev nlohmann-json3-dev libssl-dev libcrypt-dev`
- FreeBSD: `pkg install cmake fusefs-libs3 nlohmann-json`

# Building

1. Get submodules `git submodule update --init`
2. Make build folder `mkdir build; cd build`
3. Initialize cmake `cmake -DCMAKE_BUILD_TYPE="Debug|Release" ..`
4. Run compile `cmake --build .`

This is a FUSE client for an Andromeda backend file storage API.

# Usage

Run `./andromeda-fuse --help` to see the available options.
Any option or flag can also be listed in a config file named `andromeda-fuse.conf`. Example:
```
# Example config file
apiurl=http://myserv.tld/index.php
read-only
```

# Building

1. Get submodules `git submodule update --init`
2. Make build folder `mkdir build; cd build`
3. Initialize cmake `cmake -DCMAKE_BUILD_TYPE="Debug|Release" ..`
4. Run compile `cmake --build .`

### Build System

- C++17 compiler (GCC 8+ or Clang 5+)
- cmake (>= 3.18)
- Python3

### Libraries

- OpenSSL (1.1.1) (libssl, libcrypto)
- libfuse (3.x >= 3.10) https://github.com/libfuse/libfuse
    - for macOS, use OSXFUSE https://github.com/osxfuse/fuse
    - If you need to use FUSE 2.x, run cmake with `-DUSE_FUSE2=1`
- nlohmann-json (3.x) https://github.com/nlohmann/json

libssl, libcrypto, and libfuse are dynamically linked so they must be available at runtime.

Some other dependencies are included in thirdparty/ and built in-tree.

### OS Examples

- Ubuntu: `apt install make cmake g++ fuse3 libfuse3-dev nlohmann-json3-dev libssl-dev libcrypt-dev`
- Manjaro: `pacman -S make cmake gcc fuse3 nlohmann-json3`
- Alpine: `apk add make cmake g++ fuse3 fuse3-dev nlohmann-json openssl-dev`
- FreeBSD: `pkg install cmake fusefs-libs3 nlohmann-json`
- macOS: `brew install make cmake nlohmann-json openssl@1.1`
    - Install macFUSE from https://osxfuse.github.io/

# Debug

Using the `-d int` or `--debug int` option turns on debug.

1. Runs the app in the foreground and reports errors
2. Reports all calls to the backend
3. Shows many function calls and extra info (a lot!)
4. With each print shows the time, thread ID, and object ID

The `--cachemode enum` option is also useful for debugging caching.

- none - turns off caching and sends every read/write to the server (slow!)
- memory - never reads/writes the server, only memory (data loss!)
- normal - the normal mode of cache operation

* [Overview](#overview)
* [libandromeda](#libandromeda)
* [FUSE Client](#fuse-client)
* [Development](#development)

# Overview

This repo contains the desktop applications and common libraries for Andromeda's web-based file storage API.  All require that the server have the core, accounts, and files apps active.  

### Targets

There are several binaries and libraries in the full suite.  To build one individually, go to its build subdirectory after running cmake, before building.  By default, all targets will be built (see the preqrequisites sections for each!).  

- `src/lib/andromeda` the core library that implements communication with the server
- `src/bin/andromeda-fuse-cli` and `src/lib/andromeda-fuse` for mounting with FUSE
- FUTURE `src/bin/andromeda-sync-cli` and `src/lib/andromeda-sync` for running directory sync operations
- FUTURE `src/bin/andromeda-gui` a Qt-based client for FUSE and directory sync (depends on all libraries)

### Building

1. Get submodules `git submodule update --init`
2. Make build folder `mkdir build; cd build`
3. Initialize cmake `cmake -DCMAKE_BUILD_TYPE="Debug|Release" ..`
4. Run compile `cmake --build .`

### Build System

- C++17 compiler (GCC 8+ or Clang 5+)
- cmake (>= 3.16)
- Python3

### Supported Platforms

The following platforms are targeted for support and should work:
- Debian/Ubuntu: `apt install make cmake g++ python3`
  - Ubuntu 20.04 amd64 (cmake 3.16, GCC 9.4, python 3.8)
  - Ubuntu 22.10 amd64 (cmake 3.24, GCC 12.2, python 3.10)
  - Debian 11 armhf (cmake 3.18, GCC 10.2, python 3.9)
- Arch Linux amd64: `pacman -S make cmake gcc python` (cmake 3.24, GCC 12.2, python 3.10)
- Alpine Linux amd64: `apk add make cmake g++ python3` (cmake 3.23, GCC 11.2, python 3.10)
- FreeBSD amd64: `pkg install cmake python`
  - FreeBSD 12.3 (cmake 3.23, Clang 10.0, python 3.9)
  - FreeBSD 13.1 (cmake 3.23, Clang 13.0, python 3.9)
- (FUTURE) macOS amd64: `brew install make cmake python`?


# libandromeda

### Libraries

- OpenSSL (1.1.1 or 3.x) (libssl, libcrypto)
- nlohmann-json (3.x) https://github.com/nlohmann/json

libssl, libcrypto are dynamically linked so they must be available at runtime.

Some other dependencies are included in thirdparty/ and built in-tree.

### OS Examples

- Debian/Ubuntu: `apt install nlohmann-json3-dev libssl-dev libcrypt-dev` 
  - Ubuntu 20.04 (nlohmann 3.7, openssl 1.1.1f)
  - Ubuntu 22.10 (nlohmann 3.11, openssl 3.0.5)
- Arch/Manjaro: `pacman -S nlohmann-json openssl` (nlohmann 3.11, openssl 1.1.1q)
- Alpine: `apk add nlohmann-json openssl-dev` (nlohmann 3.10, openssl 1.1.1q)
- FreeBSD: `pkg install nlohmann-json`
  - FreeBSD 12: (nlohmann 3.10, openssl 1.1.1l)
  - FreeBSD 13: (nlohmann 3.10, openssl 1.1.1o)
- (FUTURE) macOS: `brew install nlohmann-json openssl@1.1`


# FUSE Client

Run `./andromeda-fuse-cli --help` to see the available options.
Authentication details (password, twofactor) will be prompted for interactively as required.
Any option or flag can also be listed in a config file named `andromeda-fuse.conf`. 
Example:
```
# Example config file
apiurl=http://myserv.tld/index.php
read-only
```

The ID of a folder to mount can also be specified in the `-s` URL.
If no folder/filesystem ID is provided, the "SuperRoot" will be mounted
containing all filesystems and other special folders.

The FUSE client can either connect to a remote server via HTTP by specifying a URL with `-s`,
or it can run the server as a local program by specifying the path with `-p`.  Using `-p` as a 
flag rather than an option will attempt to use $PATH to find the server.

When running via CLI by default it will just use the `auth_sudouser` option with the server
rather than creating and using a session.  This will make storages with encrypted credentials
inaccessible.  You can force the use of a session with `--force-session`.  This option has the
downside of potentially exposing authentication details to other processes as they are placed
on the command line in cleartext.

## Building

### Libraries

- libfuse (3.x >= 3.9 or 2.x >= 2.9) https://github.com/libfuse/libfuse
    - To compile with FUSE 2.x, run cmake with `-DUSE_FUSE2=1`
    - for macOS, use OSXFUSE https://osxfuse.github.io/

libfuse is dynamically linked so it must be available at runtime.

### OS Examples

- Ubuntu 20.04: `apt install fuse libfuse-dev` (fuse 2.9)
- Ubuntu 22.10: `apt install fuse3 libfuse3-dev` (fuse 3.11)
- Arch/Manjaro: `pacman -S fuse3` (fuse 3.12)
- Alpine: `apk add fuse3 fuse3-dev` (fuse 3.11)
- FreeBSD: `pkg install fusefs-libs3` (fuse 3.11)

Note for FreeBSD to allow FUSE mounting by regular users, you will need to add your user to the operator group with `pw group mod operator -m myuser`, and enable user mounting with `sysctl vfs.usermount=1`.  

## Debug

Using the `-d int` or `--debug int` option turns on debug.

1. Runs the app in the foreground and reports errors
2. Reports all calls to the backend
3. Shows many function calls and extra info (a lot!)
4. With each print shows the time, thread ID, and object ID

The `--cachemode enum` option is also useful for debugging caching.

- none - turns off caching and sends every read/write to the server (slow!)
- memory - never reads/writes the server, only memory (data loss!)
- normal - the normal mode of cache operation


# Development

## License

Andromeda including all source code, documentation, and APIs are copyrighted by the author.  Use of any code is licensed under the GNU GPL Version 3.  Use of any documentation (wiki, readme, etc.) is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 (CC BY-NC-SA 3.0) license.  Contributors agree that all contributions are made under the existing license.

## Doxygen

Use the `tools/mkdocs` script from the repo root to generate documentation using Doxygen.  It will output separately for each bin/lib subdirectory.  This requires `doxygen` and `graphviz`.  Use `tools/mkdocs latex` to generate LaTEX PDFs - requires `doxygen-latex`.  

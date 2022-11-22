
* [Overview](#overview)
* [libandromeda](#libandromeda)
* [FUSE Client](#fuse-client)
* [Qt GUI Client](#qt-gui-client)
* [Development](#development)

# Overview

This repo contains the desktop applications and common libraries for Andromeda's web-based file storage API.  All require that the server have the core, accounts, and files apps active.  

### Targets

There are several binaries and libraries in the full suite.  By default, all targets will be built (see the preqrequisites sections for each!).  

To build one individually, configure cmake targeted at the directory you desire to build, then make normally.  This also will only require dependencies for that target.  E.g. `cmake ../src/bin/andromeda-fuse-cli; cmake --build .`.  To only skip the Qt GUI, use `-DWITHOUT_GUI=1`.  

- `src/lib/andromeda` the core library that implements communication with the server
- `src/bin/andromeda-fuse-cli` and `src/lib/andromeda-fuse` for mounting with FUSE
- FUTURE `src/bin/andromeda-sync-cli` and `src/lib/andromeda-sync` for running directory sync operations
- `src/bin/andromeda-gui` a Qt-based GUI client for FUSE and directory sync (uses the above libraries)

### Building

Run `tools/buildrel` for a release build.  Run `tools/builddev` for a development build, including static analysis and unit tests.  Or the manual steps:

1. Make build folder `mkdir build; cd build`
2. Initialize cmake `cmake -DCMAKE_BUILD_TYPE="Debug|Release" ..`
3. Run compile `cmake --build .`

Cmake will automatically clone in-tree dependencies.

### Build System

- C++17 compiler
- cmake (>= 3.16)
- Python3 (>= 3.8?)
- Bash (if running tools/)

# libandromeda

### Libraries

- OpenSSL (1.1.1 or 3.x) (libssl, libcrypto)

libssl, libcrypto are dynamically linked so they must be available at runtime.

Some other dependencies are included in thirdparty/ and built in-tree.

### Supported Platforms

The following platforms are targeted for support and should work:

- Debian/Ubuntu: `apt install make cmake g++ python3`
  - Ubuntu 20.04 amd64 (GCC 9.4)
  - Ubuntu 22.10 amd64 (GCC 12.2 or Clang 15)
  - Debian 11 armhf (GCC 10.2)
- Arch Linux amd64: `pacman -S make cmake gcc python` (GCC 12.2 or Clang 14)
- Alpine Linux amd64: `apk add make cmake g++ python3` (GCC 11.2)
- FreeBSD amd64: `pkg install cmake python`
  - FreeBSD 12.3 (Clang 10.0)
  - FreeBSD 13.1 (Clang 13.0)
- macOS amd64: `brew install make cmake`
- Windows 10 x64 ([cmake](https://github.com/Kitware/CMake/releases/), [MSVC++](https://visualstudio.microsoft.com/downloads/) 17/2022, [python](https://www.python.org/downloads/windows/))

### OS Examples

- Debian/Ubuntu: `apt install libssl-dev libcrypt-dev` 
- Arch/Manjaro: `pacman -S openssl`
- Alpine: `apk add openssl-dev`
- FreeBSD: openSSL already installed?
- macOS: `brew install openssl@1.1`
- Windows: [OpenSSL](https://slproweb.com/products/Win32OpenSSL.html)
  - You will need to add `OPENSSL_ROOT_DIR` to your system environment


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

- libfuse (3.x >= 3.9? or 2.x >= 2.9?) https://github.com/libfuse/libfuse
    - To compile with FUSE 2.x, configure cmake with `-DLIBFUSE2=1`
    - for macOS, use OSXFUSE https://osxfuse.github.io/
    - For Windows, install WinFSP (with Developer) https://winfsp.dev/rel/

libfuse is dynamically linked so it must be available at runtime.

### OS Examples

- Ubuntu 20.04: `apt install fuse libfuse-dev` (use `-DLIBFUSE2=1`)
- Ubuntu 22.10: `apt install fuse3 libfuse3-dev`
- Arch/Manjaro: `pacman -S fuse3`
- Alpine: `apk add fuse3 fuse3-dev`
- FreeBSD: `pkg install fusefs-libs3`

Note for FreeBSD to allow FUSE mounting by regular users, you will need to add your user to the operator group with `pw group mod operator -m $(whoami)`, and enable user mounting with `sysctl vfs.usermount=1`.  

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


# Qt GUI Client

This is in VERY early demo-stage development.

## Building

### Libraries

- Qt 6.x >= 6.2 LTS
  - Qt 5.12 is supported on Ubuntu 20.04, only

### OS Examples

Official OS support for the GUI is more narrow than the other targets.  Reminder, you can configure cmake with `-DWITHOUT_GUI=1`.

- Ubuntu 20.04: `apt install qtbase5-dev`
- Ubuntu 22.10: `apt install qt6-base-dev`
- Arch/Manjaro: `pacman -S qt6-base`
- Windows: [Qt Framework](https://www.qt.io/download)

# Development

## License

Andromeda including all source code, documentation, and APIs are copyrighted by the author.  Use of any code is licensed under the GNU GPL Version 3.  Use of any documentation (wiki, readme, etc.) is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 (CC BY-NC-SA 3.0) license.  Contributors agree that all contributions are made under the existing license.

## Doxygen

Use the `tools/mkdocs` script from the repo root to generate documentation using Doxygen.  It will output separately for each bin/lib subdirectory.  This requires `doxygen` and `graphviz`.  Use `tools/mkdocs latex` to generate LaTEX PDFs - requires `doxygen-latex`.  

## Testing

Unit testing is done with catch2, which is built in-tree.  Configure cmake with `-DBUILD_TESTING=1`, then build, and the tests will be run.  Static analysis is done with cppcheck (must be installed).  Both unit tests and static analysis are part of `tools/builddev`.  Static analysis can be run standalone with `tools/analyze`.

## Catch2

Unit testing is done with catch2, which is built in-tree.  Run `tools/unittests` or configure cmake with `-DBUILD_TESTING=1`, then build.


!!! Andromeda is in early development - do not use in production! !!!

* [Overview](#overview)
* [Building](#building)
* [CLI Client](#cli-client)
* [FUSE Client](#fuse-client)
* [Qt GUI Client](#qt-gui-client)
* [Development](#development)
* [Common Usage](#common-usage)


# Overview

This repo contains the native applications and common libraries for Andromeda's web-based file storage API.  All require that the server have the core, accounts, and files apps active.  

### Targets

There are several binaries and libraries in the full suite.
- `src/lib/andromeda` the core library that implements communication with the server
- `src/bin/andromeda-cli` allows manual CLI communication with the backend
- `src/bin/andromeda-fuse` and `src/lib/andromeda-fuse` for mounting with FUSE
- FUTURE `src/bin/andromeda-sync` and `src/lib/andromeda-sync` for running directory sync operations
- `src/bin/andromeda-gui` a Qt-based GUI client for FUSE and directory sync


# Building
  
By default, all targets will be built.

To build one individually, configure cmake targeted at the directory you desire to build, then make normally.  This also will only require dependencies for that target.  E.g. `cmake ../src/bin/andromeda-fuse; cmake --build .`.  To only skip the Qt GUI, use `-DWITHOUT_GUI=1`.  

Run `tools/buildrel` for a release build.  Run `tools/builddev` for a development build including unit tests.  Or the manual steps:

1. Make build folder `mkdir build; cd build`
2. Initialize cmake `cmake -DCMAKE_BUILD_TYPE="Debug|Release" ..`
3. Run compile `cmake --build .`

Cmake will automatically clone in-tree dependencies.  By default, position-independent executables (PIE) are built.  This can be disabled with `-DWITHOUT_PIE=1`.  

### Build System

- C++17 compiler
- cmake (>= 3.16)
- Python3 (>= 3.8?)
- Bash (if running tools/)

### Prerequisite Libraries

- libandromeda
  - OpenSSL (1.1.1 or 3.x) (libssl, libcrypto)
    - dynamically linked - must be available at runtime
- libandromeda-fuse
  - libfuse (3.x >= 3.9? or 2.x >= 2.9?) https://github.com/libfuse/libfuse
    - dynamically linked - must be available at runtime
    - for macOS, use OSXFUSE https://osxfuse.github.io/
    - For Windows, install WinFSP (with Developer) https://winfsp.dev/rel/
- andromeda-gui
  - Qt (Windows/macOS: >= 6.4, Linux: >= 5.12)

Some other dependencies will be fetched by cmake and built in-tree.

### Supported Platforms

The following platforms are targeted for support and should work:

- Windows 10 x64 ([cmake](https://github.com/Kitware/CMake/releases/), [MSVC++](https://visualstudio.microsoft.com/downloads/) 17/2022, [python](https://www.python.org/downloads/windows/)), [OpenSSL](https://slproweb.com/products/Win32OpenSSL.html), [Qt Framework](https://www.qt.io/download)
  - You will need to add `OPENSSL_ROOT_DIR` to your system environment

- Debian/Ubuntu: `apt install make cmake g++ python3 libssl-dev libcrypt-dev`
  - Ubuntu 20.04 amd64 `apt install fuse libfuse-dev qtbase5-dev` (GCC 9.4)
  - Ubuntu 22.10 amd64 `apt install fuse3 libfuse3-dev qt6-base-dev` (GCC 12.2 or Clang 15)
  - Debian 11 armhf `apt install fuse3 libfuse3-dev qt6-base-dev` (GCC 10.2)
- Arch Linux amd64: `pacman -S make cmake gcc python openssl fuse3 qt6-base` (GCC 12.2 or Clang 14)

The following platforms are supported minus the Qt GUI:

- Alpine Linux amd64: `apk add make cmake g++ python3 openssl-dev fuse3-dev` (GCC 11.2)
- FreeBSD amd64: `pkg install cmake python fusefs-libs3`
  - FreeBSD 12.3 (Clang 10.0)
  - FreeBSD 13.1 (Clang 13.0)
- macOS amd64: `brew install make cmake openssl@1.1`

Note for FreeBSD to allow FUSE mounting by regular users, you will need to add your user to the operator group with `pw group mod operator -m $(whoami)`, and enable user mounting with `sysctl vfs.usermount=1`.  FreeBSD also currently has a few issues that may result in ERR#78 (Not implemented) errors on file accessat() and close().


# CLI Client

The CLI client emulates using the command-line Andromeda server API, but over a remote HTTP connection and in JSON mode.  Requires libandromeda.

Any features that rely on the higher privileges of the real CLI interface are not available. Examples:
* Changing the database configuration file
* PHP printr format (JSON only)
* Changing debug/metrics output
* Doing a request dry-run

The general usage is the same as the real CLI interface, with an added URL and different global options.  
Parameter syntax, attaching/uploading files and using environment variables is the same. Batching is not yet supported.

Run `./andromeda-cli --help` to see the available options.  

Example that shows the available API calls: `./andromeda-cli -a (url) core usage`


# FUSE Client

The FUSE client allows mounting Andromeda storage as local storage using FUSE.  Requires libandromeda and libandromeda-fuse.

Run `./andromeda-fuse --help` to see the available options.
Authentication details (password, twofactor) will be prompted for interactively as required.

The ID of a folder to mount can also be specified in the `-a` URL.
If no folder/filesystem ID is provided, the "SuperRoot" will be mounted
containing all filesystems and other special folders.

The FUSE client can either connect to a remote server via HTTP by specifying a URL with `-a`,
or it can run the server as a local program by specifying the path with `-p`.  Using `-p` as a 
flag rather than an option will attempt to use $PATH to find the server.

When running via CLI by default it will just use the `auth_sudouser` option with the server
rather than creating and using a session.  This will make storages with encrypted credentials
inaccessible.  You can force the use of a session with `--force-session`.  This option has the
downside of potentially exposing authentication details to other processes as they are placed
on the command line in cleartext.

## Debug

The `--cachemode enum` option is also useful for debugging caching.

- none - turns off caching and sends every read/write to the server (slow!)
- memory - never reads/writes the server, only memory (data loss!)
- normal - the normal mode of cache operation


# Qt GUI Client

This is in VERY early demo-stage development!

The Qt GUI client implements a GUI for mounting Andromeda storage using FUSE.   Requires libandromeda and libandromeda-fuse.


# Common Usage

Given HTTP server URLs can optionally include the protocol, the port number, and URL variables.  For example both of the following are valid: `myhostname` and `https://myhostname.tld:4430/test.php?urlvar`.  The default protocol is HTTP, so specifying `https://` ensures TLS is always used.

Any option or flag accepted on the command line can also be listed in a config file named after the binary, e.g. `andromeda-fuse.conf`. 
Example:
```
# Example config file
apiurl=myserv.tld
read-only
```

#### Runtime Debug

Using the `-d int` or `--debug int` option turns on debug.  Errors are always printed.

0. Runs the app in the foreground (if applicable)
1. Reports all calls to the backend
2. Shows many function calls and extra info (a lot!)
3. With each print shows the time, thread ID, and object ID  

Use `--debug-filter` to show debug only from the component names given, e.g. `--debug-filter FuseOperations,FuseAdapter`.  


# Development

## License

Andromeda including all source code, documentation, and APIs are copyrighted by the author.  Use of any code is licensed under the GNU GPL Version 3.  Use of any documentation (wiki, readme, etc.) is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 (CC BY-NC-SA 3.0) license.  Contributors agree that all contributions are made under the existing license.

## Doxygen

Use the `tools/mkdocs` script from the repo root to generate documentation using Doxygen.  It will output separately for each bin/lib subdirectory.  This requires `doxygen` and `graphviz`.  Use `tools/mkdocs latex` to generate LaTEX PDFs - requires `doxygen-latex`.  

## Testing

Unit testing is done with catch2, which is built in-tree.  Configure cmake with `-DBUILD_TESTING=1`, then build, and the tests will be run.  Static analysis is done with cppcheck (must be installed).  Both unit tests and static analysis are part of `tools/builddev`.  Static analysis can be run standalone with `tools/analyze`.

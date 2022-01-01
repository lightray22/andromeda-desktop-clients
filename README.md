
This repo contains the desktop applications and common libraries for Andromeda's file storage API. 

# Libraries

## Building

1. Get submodules `git submodule update --init`
2. Make build folder `mkdir build; cd build`
3. Initialize cmake `cmake -DCMAKE_BUILD_TYPE="Debug|Release" ..`
4. Run compile `cmake --build .`

To build only a specific app, go to src/bin/(app), then create the build folder and run the build from there.

### Build System

- C++17 compiler (GCC 8+ or Clang 5+)
- cmake (>= 3.18)
- Python3

### Libraries

- OpenSSL (1.1.1) (libssl, libcrypto)
- nlohmann-json (3.x) https://github.com/nlohmann/json

libssl, libcrypto are dynamically linked so they must be available at runtime.

Some other dependencies are included in thirdparty/ and built in-tree.

### OS Examples

- Ubuntu: `apt install make cmake g++ nlohmann-json3-dev libssl-dev libcrypt-dev`
- Manjaro: `pacman -S make cmake gcc nlohmann-json3`
- Alpine: `apk add make cmake g++ nlohmann-json openssl-dev`
- FreeBSD: `pkg install cmake nlohmann-json`
- macOS: `brew install make cmake nlohmann-json openssl@1.1`


# FUSE Client

Supports Linux, FreeBSD, macOS.  Located in `src/bin/andromeda-fuse-cli`.

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

- libfuse (3.x >= 3.10) https://github.com/libfuse/libfuse
    - If you need to use FUSE 2.x, run cmake with `-DUSE_FUSE2=1`
    - for macOS, use OSXFUSE https://github.com/osxfuse/fuse
        - Install from https://osxfuse.github.io/

libfuse is dynamically linked so it must be available at runtime.

### OS Examples

- Ubuntu: `apt install fuse3 libfuse3-dev`
- Manjaro: `pacman -S fuse3`
- Alpine: `apk add make fuse3 fuse3-dev`
- FreeBSD: `pkg install fusefs-libs3`

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

# ZSTREAM

ZeroMQ streaming utility. Allows send data from `STDIN` as multi-parts message
over ZMQ socket (in PUB or PUSH mode).

# Obtain

You have to compile from source or find somewhere pre-built binary.

## Build requirements

1. CMake 2.8+
2. Make
3. C  compiler
4. ZeroMQ library (at least 3+ version) with **development headers**
5. git (optional) for clone

## Runtime requirements

1. ZeroMQ library (at least 3+ version)

## Build

This is generic CMake process.

First, create new empty directory:

    mkdir -p /tmp/build-zstream
    cd /tmp

Then clone Git repo

    git clone https://github.com/reddec/zstream.git

Use CMake for prepare MAKE files:

    cd /tmp/build-zstream
    cmake -DCMAKE_BUILD_TYPE=Release /tmp/zstream

Compile and package:

    make package


### Install

After build you can use one of following method:

1. Use `zstream-1.0.0-Linux.sh` as self-installer
2. Use `zstream-1.0.0-Linux.tar.gz` as binary tar-ball
3. Use built files directly =)

# Usage

`zstream [-m mode][-c][-l line-size][-t tokens] <endpoint>`

* `endpoint`     - ZMQ endpoint string (like: `tcp://localhost:9001`)
* `-m mode`      - ZMQ socket mode: pub, sub
* `-l line-size` - positive integer as size in bytes of line buffer (default 65K)
* `-t tokens`    - characters which will be used as delimiters (same as in [strtok(3)](http://linux.die.net/man/3/strtok))
* `-s`           - become as server (bind and listen)
* `-h`           - show help
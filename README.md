# CommonAPI C++ WAMP Runtime

## Introductory remarks

CommonAPI runtime support for WAMP messaging. Please note that this project is under development.
For more information, please refer to the companion project: https://github.com/GENIVI/capicxx-wamp-tools

##### Copyright
Copyright (C) 2017, itemis AG.
Copyright (C) 2017, GENIVI Alliance, Inc.

This file is part of GENIVI Project IPC Common API C++.
Contributions are licensed to the GENIVI Alliance under
one or more Contribution License Agreements or MPL 2.0.

##### License
This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.


## Documentation

### CommonAPI WAMP C++ User Guide
As this project is currently under development, no user guide is available yet.

### Further information
https://genivi.github.io/capicxx-core-tools/

### Build Instructions for Linux

#### Prerequisites
The following packages are required to build autobahn-cpp and WAMP Runtime

- Install build tools and Boost libs: `sudo apt install libbz2-dev libssl-dev cmake libboost-dev libboost-system-dev libboost-thread-dev libboost-log-dev`

##### MsgPack-C installation
- Open your git folder in a terminal and type: `git clone https://github.com/msgpack/msgpack-c.git`
- `cd msgpack-c`
- `git checkout cpp-1.4.2`
- `cmake -DMSGPACK_CXX11=ON .`
- `sudo make install`

##### WebSocket++ installation
- Open your git folder in a terminal and type: `git clone https://github.com/zaphoyd/websocketpp.git`
- `cd websocketpp`
- `cmake .`
- `sudo make install`

##### AutobahnC++ installation
For additional help look here: [Installation on Linux](https://github.com/crossbario/autobahn-cpp).
- Open your git folder in a terminal and type: `git clone https://github.com/crossbario/autobahn-cpp.git`
- `cd autobahn-cpp`
- `sudo cp -r autobahn/ /usr/local/include/`

#### Build CommonAPI-WAMP Runtime

Use CMake to build the CommonAPI WAMP runtime library. We assume that your source directory is common-api-wamp-runtime:

```bash
$ cd common-api-wamp-runtime
$ mkdir build
$ cd build
$ cmake -D USE_INSTALLED_COMMONAPI=ON -D CMAKE_INSTALL_PREFIX=/usr/local ..
$ make
$ make install
```

You can change the installation directory by the CMake variable _CMAKE_INSTALL_PREFIX_
or you can let it uninstalled (skip the _make install_ command).
If you want to use the uninstalled version of CommonAPI set the CMake variable
_USE_INSTALLED_COMMONAPI_ to _OFF_.

For further build instructions (build for windows, build documentation, tests etc.)
please refer to the CommonAPI WAMP tutorial.

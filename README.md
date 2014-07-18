C/C++ Driver for the BBN APSv2
===============================

This repository provides the C++ (with a C calling API) driver for controlling the second generation [BBN Arbitrary Pulse Sequencer](http://quantum.bbn.com/tools/aps).  In addition to the C driver we provide thin wrappers for Matlab and Julia.  

Documentation
-------------
Further documentation can be found ???.

License
-------------
This code is licensed under the Apache v2 license.  See the LICENSE file for more information.

Building
------------
For those wishing to develop or build the driver from source. Prebuilt binaries are available the releases.

### Dependencies:

* cmake: Cmake build tool version 2.8 or higher (http://www.cmake.org/)
* gcc: g++ 4.8.1 or higher. For Windows currently building using gcc supplied with mingw-builds project (x64-4.8.1-posix-seh-rev5).  Posix thread support is important as we use C++11 std::thread. 
* hdf5 : currently built against ????
* [asio](http://think-async.com/) : We use the standalone asio package. 

### Building on Windows using MSYS2 and mingw-builds:

```
mkdir build
cd build
cmake -DHDF5_INCLUDE_DIR:STRING=/path/to/hdf5/inculde -DASIO_INCLUDE_DIR:STRING=/path/to/asio -G "MSYS Makefiles" ../src/
make
```


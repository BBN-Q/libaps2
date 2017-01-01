C/C++ Driver for the BBN APSv2
===============================

This repository provides the C++ (with a C calling API) driver for controlling the second generation [BBN Arbitrary Pulse Sequencer](http://quantum.bbn.com/tools/aps).  In addition to the C driver we provide thin wrappers for Matlab and Julia.  

Documentation
-------------
Read the [online manual](http://libaps2.readthedocs.io/).

License
-------------
This code is licensed under the Apache v2 license.  See the LICENSE file for more information.

Building
------------
For those wishing to develop or build the driver from source for use on Linux or OS X. Prebuilt binaries for Windows are available on the releases tab.

### Clone repository
We get the [asio](http://think-async.com/Asio) dependency via a submodule we need the --recursive switch

  ```bash
  git clone --recursive git@github.com:BBN-Q/libaps2.git
  ```

### Dependencies

* C++ compiler with good C++11 support. See below for OS specific compilers tested.
* [cmake](http://www.cmake.org/): Cmake build tool version 2.8 or higher (http://www.cmake.org/)
* [hdf5](http://www.hdfgroup.org/HDF5/): Currently built against 1.8.13.  Watch out for HDF5 version incompatibilities with other programs (such as Matlab) that ship with a bundled HDF5 library.  You may have to use the environment variable ``HDF5_DISABLE_VERSION_CHECK=1`` to avoid conflict.

### Windows
Using gcc on Windows is a rapidly moving target.  Our setup has changed every couple of months but the latest and most painless way has been using [MSYS2](http://sourceforge.net/projects/msys2/) and the [MinGW-w64](http://mingw-w64.sourceforge.net/) gcc compiler stack.

1. [Download](http://msys2.github.io/) the MSYS2 installer and follow the instructions to install and update in place.
2. Use pacman package manager to install some additional tools and libraries:

  ```bash
  pacman -S make
  pacman -S mingw64/mingw-w64-x86_64-cmake
  pacman -S mingw64/mingw-w64-x86_64-gcc
  pacman -S mingw64/mingw-w64-x86_64-gdb
  pacman -S mingw64/mingw-w64-x86_64-hdf5
  ```
3. Finally build in the libaps2 folder

  ```
  mkdir build
  cd build
  cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug ../src/
  make
  ```

Tested on:
* Windows 7 Professional with gcc 4.9.2
* Windows 10 Professional with gcc 5.2.0

### Linux
Use your distribution's package manager to install the dependencies and it should work out of the box.

Tested on:
* Linux Mint 17.3 with gcc 4.7, 4.8

### OS X
1. Install the command-line developer tools.
2. Use [Homebrew](http://brew.sh/) to install hdf5 and cmake. For HDF5 you'll need to tap the science formulae and build the C++ bindings:

    ```bash
    brew tap homebrew/science
    brew install hdf5 --enable-cxx
    ```
3. Then a standard ``cmake ../src`` and ``make`` should build.

Tested on:
* OS X 10.10 Yosemite with Apple clang 6.0

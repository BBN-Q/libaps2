C/C++ Driver for the BBN APSv2
===============================

This repository provides the C++ (with a C calling API) driver for controlling the second generation [BBN Arbitrary Pulse Sequencer](http://quantum.bbn.com/tools/aps).  In addition to the C driver we provide thin wrappers for Matlab and Julia.  

Documentation
-------------
Read the [online manual](http://libaps2.readthedocs.org/).

License
-------------
This code is licensed under the Apache v2 license.  See the LICENSE file for more information.

Building
------------
For those wishing to develop or build the driver from source for use on Linux or OS X. Prebuilt binaries for Windows are available on the releases tab.

### Clone repository
We get the [asio] dependency via a submodule we need the --recursive switch

  ```bash
  git clone --recursive git@github.com:BBN-Q/libaps2.git
  ```

### Dependencies

* C++ compiler: Threaded support is important as we use C++11 std::thread. See below for OS specific compilers tested.
* [cmake](http://www.cmake.org/): Cmake build tool version 2.8 or higher (http://www.cmake.org/)
* [hdf5](http://www.hdfgroup.org/HDF5/): Currently built against 1.8.13

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
* Windows 7 Professional
* Windows 8.1

### Linux
Use your distribution's package manager to install the dependencies and it should work out of the box.

Tested on:
* Linux Mint 16

### OS X
1. Install the command-line developer tools. 
2. Use [Homebrew](http://brew.sh/) to install hdf5 and cmake. For HDF5 you'll need to tap the science formulae and build the C++ bindings:

    ```bash
    brew tap homebrew/science
    brew install hdf5 --enable-cxx
    ```
3. Then a standard ``cmake ../src`` and ``make`` should build. 

Tested on:
* OS X 10.9 Mavericks

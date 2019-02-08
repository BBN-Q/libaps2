C/C++ Driver for the BBN APSv2
===============================

[![Build status](https://ci.appveyor.com/api/projects/status/96bjuekewvan9xry?svg=true)](https://ci.appveyor.com/project/caryan/libaps2)

This repository provides the C++ (with a C calling API) driver for controlling the second generation [BBN Arbitrary Pulse Sequencer](http://quantum.bbn.com/tools/aps).  In addition to the C driver we provide thin wrappers for Matlab and Julia.  

Documentation
-------------
Read the [online manual](http://libaps2.readthedocs.io/).

License
-------------
This code is licensed under the Apache v2 license.  See the LICENSE file for more information.

Building
------------
Only necessary for those wishing to develop or build the driver from source.
Prebuilt binaries for Windows are available on the releases tab or for Python
from the [conda package manager](https://anaconda.org/BBN-Q/libaps2).

### Clone repository
We get the [asio](http://think-async.com/Asio) dependency via a submodule we need the --recursive switch

  ```bash
  git clone --recursive git@github.com:BBN-Q/libaps2.git
  ```

### Dependencies

* C++ compiler with good C++11 support. See below for OS specific compilers tested.
* [cmake](http://www.cmake.org/): Cmake build tool version 2.8 or higher (http://www.cmake.org/)

### Windows

#### Visual Studio

We have built libaps2 with Visual Studio 2015 using the
"CMake VS 2015 C, C++, IVF 16" version. In PowerShell with git available create
the `version.hpp` file: ``cat .\src\lib\version_template.hpp | % {$_ -replace
"<TOKEN>", "$(git describe --dirty)"} | Set-Content .\src\lib\version.hpp``.
Then in the "VS2015 x64 Native Tools" command prompt

```cmd
md build
cd build
cmake -G "Visual Studio 14 2015 Win64" ..\src
cmake --build . --config Release
```

#### MSYS2 and MinGW-w64

The most painless way to use gcc on Windows has been using
[MSYS2](http://sourceforge.net/projects/msys2/) and the
[MinGW-w64](http://mingw-w64.sourceforge.net/) gcc compiler stack.

1. [Download](http://msys2.github.io/) the MSYS2 installer and follow the instructions to install and update in place.
2. Use pacman package manager to install some additional tools and libraries:

  ```bash
  pacman -S make
  pacman -S mingw64/mingw-w64-x86_64-cmake
  pacman -S mingw64/mingw-w64-x86_64-gcc
  pacman -S mingw64/mingw-w64-x86_64-gdb
  ```
3. Finally build in the libaps2 folder

  ```
  mkdir build
  cd build
  cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug ../src/
  make
  ```

Tested on:
* Windows 10 Professional with gcc 6.1.0

### Linux
Use your distribution's package manager to install the dependencies and it should work out of the box.

Tested on:
* Linux Mint 18.1 with 5.4.0

### OS X
1. Install the command-line developer tools.
2. Use [Homebrew](http://brew.sh/) to install cmake. 
3. Then a standard ``cmake ../src`` and ``make`` should build.

Tested on:
* OS X 10.10 Yosemite with Apple clang 6.0

## Funding

This software was funded in part by the Office of the Director of National
Intelligence (ODNI), Intelligence Advanced Research Projects Activity (IARPA),
through the Army Research Office contract No. W911NF-10-1-0324 and No.
W911NF-14-1-0114. All statements of fact, opinion or conclusions contained
herein are those of the authors and should not be construed as representing the
official views or policies of IARPA, the ODNI, or the US Government.

# Build instructions (from project root folder):
#
# mkdir build
# cd build
# cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release/Debug ../src
# make
#
# On Mac/Linux you can skip the '-G "MSYS Makefiles" option'
#
# If your HDF5 install is in a non-standard location set the HDF5_ROOT environment variable

cmake_minimum_required(VERSION 3.1)

project( libaps2 )

# Make sure we have C++11 support
set(CMAKE_CXX_STANDARD 11)

# build the library version string from git
execute_process(
  COMMAND git describe --dirty
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string( REGEX MATCH "^v([0-9]+)\.([0-9]+)" VERSION_STRING ${GIT_DESCRIBE} )

if (NOT VERSION_STRING)
  message(WARNING "Failed to extract version string from 'git describe'")
endif()
set(APS2_VERSION_MAJOR ${CMAKE_MATCH_1})
set(APS2_VERSION_MINOR ${CMAKE_MATCH_2})
set(APS2_VERSION_PATCH 0)
set(APS2_VERSION_STRING ${APS2_VERSION_MAJOR}.${APS2_VERSION_MINOR}.${APS2_VERSION_PATCH})

if(MSVC)
    # look for HDF5 using the HDF5 provided hdf5-config.cmake
    # only works on Windows for now because there both Anaconda and HDF5 provide hdf5-config.cmake
    find_package (HDF5 COMPONENTS C CXX NO_MODULE REQUIRED static)
    include_directories (${HDF5_INCLUDE_DIR})
    set (HDF5_LINK_LIBS ${HDF5_CXX_STATIC_LIBRARY})
else()
    # otherwise fall back to CMake's built-in FindHDF5.cmake logic
    find_package(HDF5 REQUIRED COMPONENTS C CXX)
    include_directories( ${HDF5_INCLUDE_DIRS} )
    set (HDF5_LINK_LIBS ${HDF5_CXX_LIBRARIES})
    set (HDF5_INCLUDE_DIR ${HDF5_INCLUDE_DIRS})
endif()

if(HDF5_FOUND)
    message("Found HDF5 version ${HDF5_VERSION} and libraries ${HDF5_LINK_LIBS} with headers in ${HDF5_INCLUDE_DIR}")
endif()

# look for threading library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Grab all the header files in the src directory including concol.h
include_directories("./lib/")

# ASIO
include_directories( "../deps/asio/asio/include" )
add_definitions(-DASIO_STANDALONE)

# Catch unit-testing
include_directories("../deps/Catch/include")

# mingw64 defaults to WindowsXP SP03 which doesn't support the network calls we want
# https://github.com/Alexpux/mingw-w64/blob/e960f8fffb399036079cef960a44e86a9abebc71/mingw-w64-headers/include/sdkddkver.h#L145
# and asio needs the Windows version defined
# define windows 7
if(WIN32)
    add_definitions(${CMAKE_CXX_FLAGS} "-D_WIN32_WINNT=0x0601")
endif()

# don't let windows.h clobber std::min and std::max
if(MSVC)
    add_definitions(-DNOMINMAX)
endif()

# Warn all for better code but disable some Visual Studio ones for now
# warning C4127: conditional expression is constant - shows up in all logging statements
# warning C4201: nonstandard extension used: nameless struct/union
# warning C4244: 'initializing': conversion from 'unsigned int' to 'unsigned short', possible loss of data
# warning C4267: 'argument': conversion from 'size_t' to 'uint32_t', possible loss of data
# warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.
#     however can't use "_s" safe versions with glibc
if(MSVC)
    add_definitions(${CMAKE_CXX_FLAGS} "/W3 /wd4127 /wd4201 /wd4244 /wd4267 /wd4996")
else()
    add_definitions(${CMAKE_CXX_FLAGS} "-Wall")
endif()

set ( DLL_SRC
    ./lib/libaps2.cpp
    ./lib/Channel.cpp
    ./lib/APS2.cpp
    ./lib/APS2Ethernet.cpp
    ./lib/APS2Datagram.cpp
    ./lib/MACAddr.cpp
    ./lib/APS2EthernetPacket.cpp
)

set_source_files_properties( ${DLL_SRC} PROPERTIES LANGUAGE CXX )

add_custom_target( update_version
    COMMAND ${PROJECT_SOURCE_DIR}/make_version.sh
)

add_library( aps2 SHARED ${DLL_SRC} )
add_dependencies( aps2 update_version )

set_target_properties(aps2 PROPERTIES
    VERSION ${APS2_VERSION_STRING}
    SOVERSION ${APS2_VERSION_MAJOR}
    PREFIX "lib" # force lib prefix even on Windows
    MACOSX_RPATH 1
)

foreach(target enumerate play_waveform play_sequence)
    add_executable(${target} ./C++/${target}.cpp)
endforeach()

add_executable(run_tests
    ../test/run_tests.cpp
    ../test/test_Sanity.cpp
    ../test/test_datagram.cpp
    ./lib/APS2Datagram.cpp
    ../test/RandomHelpers.cpp
    ../test/test_comms.cpp
    ../test/test_cache.cpp
    ../test/APS2Connector.cpp
    ../test/test_CSR.cpp
    ../test/test_DACs.cpp
)
target_link_libraries(run_tests aps2 Threads::Threads)

foreach(target flash reset program dac_bist)
    add_executable(${target} ./util/${target}.cpp)
endforeach()

set(BIN_TARGETS enumerate play_waveform play_sequence flash reset program dac_bist run_tests)

# add aps2_ prefix to binary targets
foreach(target ${BIN_TARGETS})
    set_target_properties(${target} PROPERTIES LINK_LIBRARIES aps2 OUTPUT_NAME aps2_${target})
endforeach()

if(WIN32)
    target_link_libraries(aps2 ${HDF5_LINK_LIBS} ws2_32 iphlpapi Threads::Threads)
else()
    target_link_libraries(aps2 ${HDF5_LINK_LIBS} Threads::Threads)
endif()

# installation section
install(TARGETS ${BIN_TARGETS} aps2
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(FILES "${PROJECT_SOURCE_DIR}/lib/libaps2.h"
    DESTINATION include)

# On linux we need to ship the system libraries
if(UNIX AND NOT APPLE)
    get_filename_component(GCC_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
    find_library(LIBGCC NAMES libgcc_s.so.1 HINTS ${GCC_PATH}/../lib64)
    get_filename_component(LIBGCC_DIR ${LIBGCC} DIRECTORY)
    file(GLOB LIBGCC_ALL ${LIBGCC_DIR}/libgcc*.so.1)
    find_library(STDCPP NAMES libstdc++.so.6 HINTS ${GCC_PATH}/../lib64)
    file(GLOB STDCPP_ALL ${STDCPP}*)
    file(GLOB HDF5LIB_ALL ${HDF5_C_LIBRARIES}*)
    # the CPP lib seems to be the first library listed in HDF5_CXX_LIBRARIES
    list(GET HDF5_CXX_LIBRARIES 0 HDF5CPPLIB)
    file(GLOB HDF5CPPLIB_ALL ${HDF5CPPLIB}*)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
    ${LIBGCC_ALL}
    ${STDCPP_ALL}
    ${HDF5LIB_ALL}
    ${HDF5CPPLIB_ALL})
endif()

# We need to ship libraries on windows, too, with slightly tweaked file names
if(WIN32 AND NOT MSVC)
    GET_FILENAME_COMPONENT(GCC_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
    FIND_LIBRARY(LIBGCC NAMES libgcc_s_seh-1.dll HINTS ${GCC_PATH})
    FIND_LIBRARY(STDCPP NAMES libstdc++-6.dll HINTS ${GCC_PATH})
    FIND_LIBRARY(HDF5LIB NAMES libhdf5-0.dll HINTS ${GCC_PATH})
    FIND_LIBRARY(HDF5CPPLIB NAMES libhdf5_cpp-0.dll HINTS ${GCC_PATH})
    FIND_LIBRARY(LIBSZIP NAMES libszip-0.dll HINTS ${GCC_PATH})
    FIND_LIBRARY(ZLIB NAMES zlib1.dll HINTS ${GCC_PATH})
    FIND_LIBRARY(LIPWINPTHREAD NAMES libwinpthread-1.dll HINTS ${GCC_PATH})
    SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
    ${LIBGCC}
    ${STDCPP}
    ${HDF5LIB}
    ${HDF5CPPLIB}
    ${LIBSZIP}
    ${ZLIB}
    ${LIPWINPTHREAD}
    ${CMAKE_THREAD_LIBS_INIT})
endif()

if(NOT ${CONDA_BUILD})
    INCLUDE( InstallRequiredSystemLibraries )
endif()

if(APPLE)
    set_target_properties(${BIN_TARGETS} PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set_target_properties(aps2 PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
    set_target_properties(${BIN_TARGETS} PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set_target_properties(aps2 PROPERTIES INSTALL_RPATH "\$ORIGIN")
endif()

# Basic "package" target
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_VERSION_MAJOR ${APS2_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${APS2_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${APS2_VERSION_PATCH})
include(CPack)

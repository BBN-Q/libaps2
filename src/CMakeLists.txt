# Build instructions (from project root folder):
#
# mkdir build
# cd build
# cmake -G "MSYS Makefiles" -DHDF5_INCLUDE_DIR:string=/path/to/hdf5 -DCMAKE_BUILD_TYPE=Release/Debug ../src
#
# make
#
# On Mac/Linux you can skip the '-G "MSYS Makefiles" option'

PROJECT( libaps2 )
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

set(APS2_VERSION_MAJOR 1)
set(APS2_VERSION_MINOR 0)
set(APS2_VERSION_PATCH 0)
set(APS2_VERSION_STRING ${APS2_VERSION_MAJOR}.${APS2_VERSION_MINOR}.${APS2_VERSION_PATCH})

#Look for hdf5 directly passed; otherwise look for system one
if(NOT DEFINED HDF5_INCLUDE_DIR)
	FIND_PACKAGE(HDF5)
else()
	LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${HDF5_INCLUDE_DIR}/../lib)
endif()
INCLUDE_DIRECTORIES( ${HDF5_INCLUDE_DIR} )
INCLUDE_DIRECTORIES( ${HDF5_INCLUDE_DIR}/cpp )

#Grab all the header files in the src directory including concol.h
INCLUDE_DIRECTORIES("./lib/")

#ASIO
INCLUDE_DIRECTORIES( "../deps/asio/asio/include" )
ADD_DEFINITIONS(-DASIO_STANDALONE)

#Catch unit-testing
INCLUDE_DIRECTORIES("../deps/Catch/include")

#Add flag for C++11
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-std=c++11")

#mingw64 defaults to WindowsXP SP03 which doesn't support the network calls we want
#https://github.com/Alexpux/mingw-w64/blob/e960f8fffb399036079cef960a44e86a9abebc71/mingw-w64-headers/include/sdkddkver.h#L145
#define windows 7 instead
if(WIN32)
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-D_WIN32_WINNT=0x0601")
endif(WIN32)

#Warn all for better code
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-Wall")

#Work around a gcc-4.7 issue with sleep_for
#See: http://stackoverflow.com/questions/4438084/stdthis-threadsleep-for-and-gcc
#Drop once Matlab supports gcc 4.8 on Linux
message(STATUS, "Compiler ID: " ${CMAKE_CXX_COMPILER_ID} " " ${CMAKE_CXX_COMPILER_VERSION})
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
		message(STATUS, "Found GCC 4.7.3 compiler so adding D_GLIBCXX_USE_NANOSLEEP workaround.")
		ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-D_GLIBCXX_USE_NANOSLEEP")
  endif()
endif()

SET ( DLL_SRC
	./lib/libaps2.cpp
	./lib/Channel.cpp
	./lib/APS2.cpp
	./lib/APS2Ethernet.cpp
	./lib/APS2Datagram.cpp
	./lib/MACAddr.cpp
	./lib/APS2EthernetPacket.cpp
)

SET_SOURCE_FILES_PROPERTIES( ${DLL_SRC} PROPERTIES LANGUAGE CXX )

ADD_LIBRARY( aps2 SHARED ${DLL_SRC} )

set_target_properties(aps2 PROPERTIES
	VERSION ${APS2_VERSION_STRING}
	SOVERSION ${APS2_VERSION_MAJOR}
	MACOSX_RPATH 1)

ADD_EXECUTABLE(enumerate
	./C++/enumerate.cpp
)

ADD_EXECUTABLE(play_waveform
	./C++/play_waveform.cpp
)

ADD_EXECUTABLE(play_sequence
	./C++/play_sequence.cpp
)

ADD_EXECUTABLE(run_tests
	../test/run_tests.cpp
	../test/test_Sanity.cpp
	../test/test_datagram.cpp
	./lib/APS2Datagram.cpp
	../test/RandomHelpers.cpp
	../test/test_comms.cpp
	../test/test_cache.cpp
)
TARGET_LINK_LIBRARIES(run_tests aps2 pthread)

ADD_EXECUTABLE(flash
	./util/flash.cpp
)

ADD_EXECUTABLE(reset
	./util/reset.cpp
)

ADD_EXECUTABLE(program
	./util/program.cpp
)

ADD_EXECUTABLE(dac_bist
	./util/DAC_BIST.cpp
)

TARGET_LINK_LIBRARIES(enumerate aps2)
TARGET_LINK_LIBRARIES(play_waveform aps2)
TARGET_LINK_LIBRARIES(play_sequence aps2)
TARGET_LINK_LIBRARIES(flash aps2)
TARGET_LINK_LIBRARIES(reset aps2)
TARGET_LINK_LIBRARIES(program aps2)
TARGET_LINK_LIBRARIES(dac_bist aps2)

SET(bin_targets enumerate play_waveform play_sequence flash reset program dac_bist run_tests)

if(WIN32)
TARGET_LINK_LIBRARIES(aps2 hdf5	hdf5_cpp ws2_32 iphlpapi pthread)
else(WIN32)
TARGET_LINK_LIBRARIES(aps2 hdf5	hdf5_cpp pthread)
endif(WIN32)

# installation section
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	SET(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install" CACHE PATH "default install path" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

INSTALL(TARGETS ${bin_targets} aps2
	    LIBRARY DESTINATION lib
	    RUNTIME DESTINATION bin)

INSTALL(FILES "${PROJECT_SOURCE_DIR}/lib/libaps2.h"
	    DESTINATION include)

# On linux we need to ship the system libraries
# TODO: do something similar for windows
IF(UNIX AND NOT APPLE)
	GET_FILENAME_COMPONENT(GCC_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
	FIND_LIBRARY(LIBGCC NAMES libgcc_s.so.1 HINTS ${GCC_PATH}/../lib64)
	FIND_LIBRARY(STDCPP NAMES libstdc++.so.6 HINTS ${GCC_PATH}/../lib64)
	FILE(GLOB STDCPP_ALL ${STDCPP}*)
	FIND_LIBRARY(HDF5LIB NAMES hdf5 HINTS ${HDF5_INCLUDE_DIR}/../lib)
	FILE(GLOB HDF5LIB_ALL ${HDF5LIB}.*)
	FIND_LIBRARY(HDF5CPPLIB NAMES hdf5_cpp HINTS ${HDF5_INCLUDE_DIR}/../lib)
	FILE(GLOB HDF5CPPLIB_ALL ${HDF5CPPLIB}.*)
	SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
		${LIBGCC}
		${STDCPP_ALL}
		${HDF5LIB_ALL}
		${HDF5CPPLIB_ALL})
ENDIF()
INCLUDE( InstallRequiredSystemLibraries )

if(APPLE)
	set_target_properties(${bin_targets} PROPERTIES INSTALL_RPATH "@loader_path/../lib")
	set_target_properties(aps2 PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
	set_target_properties(${bin_targets} PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
	set_target_properties(aps2 PROPERTIES INSTALL_RPATH "\$ORIGIN")
endif()

# Basic "package" target
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_PACKAGE_VERSION_MAJOR ${APS2_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${APS2_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${APS2_VERSION_PATCH})
INCLUDE(CPack)

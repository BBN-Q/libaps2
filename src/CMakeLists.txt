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

CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT( libaps2 )

# Make sure we have C++11 support
set (CMAKE_CXX_STANDARD 11)

set(APS2_VERSION_MAJOR 1)
set(APS2_VERSION_MINOR 2)
set(APS2_VERSION_PATCH 0)
set(APS2_VERSION_STRING ${APS2_VERSION_MAJOR}.${APS2_VERSION_MINOR}.${APS2_VERSION_PATCH})

# look for HDF5
FIND_PACKAGE(HDF5 REQUIRED COMPONENTS C CXX)
INCLUDE_DIRECTORIES( ${HDF5_INCLUDE_DIRS} )

# look for threading library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Grab all the header files in the src directory including concol.h
INCLUDE_DIRECTORIES("./lib/")

# ASIO
INCLUDE_DIRECTORIES( "../deps/asio/asio/include" )
ADD_DEFINITIONS(-DASIO_STANDALONE)

# Catch unit-testing
INCLUDE_DIRECTORIES("../deps/Catch/include")

# mingw64 defaults to WindowsXP SP03 which doesn't support the network calls we want
# https://github.com/Alexpux/mingw-w64/blob/e960f8fffb399036079cef960a44e86a9abebc71/mingw-w64-headers/include/sdkddkver.h#L145
# define windows 7 instead
if(WIN32)
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-D_WIN32_WINNT=0x0601")
endif(WIN32)

# Warn all for better code
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-Wall")

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

ADD_CUSTOM_TARGET( update_version
	COMMAND ${PROJECT_SOURCE_DIR}/make_version.sh
)

ADD_LIBRARY( aps2 SHARED ${DLL_SRC} )
ADD_DEPENDENCIES( aps2 update_version )

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
	../test/APS2Connector.cpp
	../test/test_CSR.cpp
	../test/test_DACs.cpp
)
TARGET_LINK_LIBRARIES(run_tests aps2 Threads::Threads)

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
TARGET_LINK_LIBRARIES(aps2 ${HDF5_CXX_LIBRARIES} ws2_32 iphlpapi Threads::Threads)
else(WIN32)
TARGET_LINK_LIBRARIES(aps2 ${HDF5_CXX_LIBRARIES} Threads::Threads)
endif(WIN32)

# installation section

# IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
# 	SET(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install"
# 		CACHE PATH "default install path" FORCE)
# ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

INSTALL(TARGETS ${bin_targets} aps2
			LIBRARY DESTINATION lib
			RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)

INSTALL(FILES "${PROJECT_SOURCE_DIR}/lib/libaps2.h"
			DESTINATION include)

# On linux we need to ship the system libraries
IF(UNIX AND NOT APPLE)
	GET_FILENAME_COMPONENT(GCC_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
	FIND_LIBRARY(LIBGCC NAMES libgcc_s.so.1 HINTS ${GCC_PATH}/../lib64)
	get_filename_component(LIBGCC_DIR ${LIBGCC} DIRECTORY)
	FILE(GLOB LIBGCC_ALL ${LIBGCC_DIR}/libgcc*.so.1)
	FIND_LIBRARY(STDCPP NAMES libstdc++.so.6 HINTS ${GCC_PATH}/../lib64)
	FILE(GLOB STDCPP_ALL ${STDCPP}*)
	FILE(GLOB HDF5LIB_ALL ${HDF5_C_LIBRARIES}*)
	# the CPP lib seems to be the first library listed in HDF5_CXX_LIBRARIES
	LIST(GET HDF5_CXX_LIBRARIES 0 HDF5CPPLIB)
	FILE(GLOB HDF5CPPLIB_ALL ${HDF5CPPLIB}*)
	SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
		${LIBGCC_ALL}
		${STDCPP_ALL}
		${HDF5LIB_ALL}
		${HDF5CPPLIB_ALL})
ENDIF()

# We need to ship libraries on windows, too, with slightly tweaked file names
IF(WIN32)
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
ENDIF()

IF(NOT ${CONDA_BUILD})
	INCLUDE( InstallRequiredSystemLibraries )
ENDIF()

if(APPLE)
	# set_target_properties(${bin_targets} PROPERTIES INSTALL_RPATH "@loader_path/../lib")
	set_target_properties(aps2 PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
	# set_target_properties(${bin_targets} PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
	set_target_properties(aps2 PROPERTIES INSTALL_RPATH "\$ORIGIN")
endif()

# Basic "package" target
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_PACKAGE_VERSION_MAJOR ${APS2_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${APS2_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${APS2_VERSION_PATCH})
INCLUDE(CPack)

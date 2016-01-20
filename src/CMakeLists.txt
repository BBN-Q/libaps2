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
INCLUDE_DIRECTORIES( "../asio/asio/include" )
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
	./lib/APSEthernet.cpp
	./lib/MACAddr.cpp
	./lib/APSEthernetPacket.cpp
)

SET_SOURCE_FILES_PROPERTIES( ${DLL_SRC} PROPERTIES LANGUAGE CXX )

ADD_LIBRARY( aps2 SHARED ${DLL_SRC} )

set(APS2_VERSION_MAJOR 0)
set(APS2_VERSION_MINOR 6)
set(APS2_VERSION_PATCH 0)
set(APS2_VERSION_STRING ${APS2_VERSION_MAJOR}.${APS2_VERSION_MINOR}.${APS2_VERSION_PATCH})
set_target_properties(aps2 PROPERTIES VERSION ${APS2_VERSION_STRING} SOVERSION ${APS2_VERSION_MAJOR})

ADD_EXECUTABLE(play_waveform
	./C++/play_waveform.cpp
)
TARGET_LINK_LIBRARIES(play_waveform aps2)

ADD_EXECUTABLE(play_sequence
	./C++/play_sequence.cpp
)
TARGET_LINK_LIBRARIES(play_sequence aps2)

ADD_EXECUTABLE(test_comms
	./util/test_comms.cpp
)
TARGET_LINK_LIBRARIES(test_comms aps2)

ADD_EXECUTABLE(run_tests
	../test/run_tests.cpp
	../test/test_Sanity.cpp
	../test/test_comms.cpp
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

ADD_EXECUTABLE(DAC_BIST
	./util/DAC_BIST.cpp
)

ADD_EXECUTABLE(waveforms
	./C++/waveforms.cpp
)

TARGET_LINK_LIBRARIES(flash aps2)
TARGET_LINK_LIBRARIES(reset aps2)
TARGET_LINK_LIBRARIES(program aps2)
TARGET_LINK_LIBRARIES(DAC_BIST aps2)
TARGET_LINK_LIBRARIES(waveforms aps2)

if(WIN32)
TARGET_LINK_LIBRARIES(aps2 hdf5	hdf5_cpp ws2_32 iphlpapi)
else(WIN32)
TARGET_LINK_LIBRARIES(aps2 hdf5	hdf5_cpp)
endif(WIN32)

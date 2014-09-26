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

#Add flags for gcc and C++11
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-DGCC")
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-std=c++11")
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-Wall")

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

ADD_EXECUTABLE(test
	./C++/main.cpp
)

ADD_EXECUTABLE(waveforms
	./C++/waveforms.cpp
)

ADD_EXECUTABLE(flash
	./util/flash.cpp
)

ADD_EXECUTABLE(reset
	./util/reset.cpp
)

ADD_EXECUTABLE(program
	./util/program.cpp
)

TARGET_LINK_LIBRARIES(test aps2)
TARGET_LINK_LIBRARIES(flash aps2)
TARGET_LINK_LIBRARIES(waveforms aps2)
TARGET_LINK_LIBRARIES(reset aps2)
TARGET_LINK_LIBRARIES(program aps2)

if(WIN32)
TARGET_LINK_LIBRARIES(aps2 hdf5	hdf5_cpp ws2_32)
else(WIN32)
TARGET_LINK_LIBRARIES(aps2 hdf5	hdf5_cpp)
endif(WIN32)





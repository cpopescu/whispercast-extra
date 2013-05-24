# Main cmake configuration file - we set a bunch of things here ..
#
# To build the project(s), run in this directory:
#    cmake .
#    make
#
# To build in Debug mode:
#    cmake . -DCMAKE_BUILD_TYPE=Debug
#    make
#
# Specify the install dir with:
#    cmake -DINSTALL_PREFIX=<prefix_dir> ...
#
# You can continue with testing:
#    make test
# And installation of the binaries:
#    make install
#
######################################################################

project (whispersoft)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_MICRO "1")
set(VERSION_PATCH "0")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Private whisper soft stuff")

cmake_minimum_required(VERSION 2.4)

IF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)
  CMAKE_POLICY(SET CMP0002 NEW)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)

######################################################################

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(WhisperLib REQUIRED)
find_package(WhisperStreamLib REQUIRED)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${whisperlib_INCLUDE_DIR})
include_directories(${whisperstreamlib_INCLUDE_DIR})

######################################################################

include(../cmake/common.cmake)
link_libraries(${COMMON_LINK_LIBRARIES})

ADD_SUBDIRECTORY (flash_policy_server)
ADD_SUBDIRECTORY (osd)
ADD_SUBDIRECTORY (utils)
ADD_SUBDIRECTORY (extra_library)
ADD_SUBDIRECTORY (whispercmd)
ADD_SUBDIRECTORY (whisperproc2)
if (NOT APPLE)
  ADD_SUBDIRECTORY (features)
  ADD_SUBDIRECTORY (stats)
endif (NOT APPLE)
ADD_SUBDIRECTORY (transcoder)
IF ( EXISTS test )
  ADD_SUBDIRECTORY (test)
ENDIF ( EXISTS test )

######################################################################

if (WITH_ADMIN)
  ADD_SUBDIRECTORY (admin)
endif (WITH_ADMIN)


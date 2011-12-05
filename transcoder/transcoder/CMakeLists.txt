# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Whispersoft s.r.l. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
project (whispersoft)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_MICRO "1")
set(VERSION_PATCH "0")

cmake_minimum_required(VERSION 2.4)

IF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)
  CMAKE_POLICY(SET CMP0002 NEW)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)

######################################################################

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(WhisperLib REQUIRED)
find_package(WhisperMediaLib REQUIRED)
find_package(WhisperStreamLib REQUIRED)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../../)
include_directories(${whisperlib_INCLUDE_DIR})
include_directories(${whispermedialib_INCLUDE_DIR})
include_directories(${whisperstreamlib_INCLUDE_DIR})


######################################################################

include(FindPkgConfig)

if ($ENV{PKG_CONFIG_PATH})
  set(ENV{PKG_CONFIG_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/gstreamer/__out/lib/gstreamer/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
else ($ENV{PKG_CONFIG_PATH})
  set(ENV{PKG_CONFIG_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/gstreamer/__out/lib/gstreamer/lib/pkgconfig")
endif ($ENV{PKG_CONFIG_PATH})

pkg_check_modules(GST REQUIRED gstreamer-0.10)

include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})

pkg_check_modules(GST_DP gstreamer-dataprotocol-0.10)
if (NOT GST_DP_FOUND)
  message (FATAL_ERROR "It seems that the required GStreamer libraries "
    "are not available.")
endif (NOT GST_DP_FOUND)

include_directories(${GST_DP_INCLUDE_DIRS})
link_directories(${GST_DP_LIBRARY_DIRS})

find_package(LibAvCodec REQUIRED)
find_package(LibSwScale REQUIRED)

include_directories(${AvCodec_INCLUDE_DIR})
include_directories(${SwScale_INCLUDE_DIR})
# Safeguard for some older ffmpeg versions
include_directories(${AvCodec_INCLUDE_DIR}/ffmpeg)

add_definitions(-D __AV_CODEC_INCLUDE_FILE="<${AV_CODEC_INCLUDE_FILE}>")
add_definitions(-D __SW_SCALE_INCLUDE_FILE="<${SW_SCALE_INCLUDE_FILE}>")

######################################################################

include(../../../cmake/common.cmake)
link_libraries(${COMMON_LINK_LIBRARIES})

################################################################################
#
# transcoder
#

ADD_EXECUTABLE(transcoder
  media_transcoder.cc)
ADD_DEPENDENCIES(transcoder
  whisper_lib
  whisper_medialib
  whisper_streamlib
  whisper_featurelib)
TARGET_LINK_LIBRARIES(transcoder
  media_manager_common
  ${whisperlib_LIBRARY}
  ${whispermedialib_LIBRARY}
  ${whisperstreamlib_LIBRARY}
  whisper_featurelib
  ${COMMON_LINK_LIBRARIES})

INSTALL (TARGETS transcoder
         DESTINATION bin)

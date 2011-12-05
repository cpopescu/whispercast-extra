# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#

project (whispersoft)

######################################################################

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(WhisperLib REQUIRED)
find_package(WhisperStreamLib REQUIRED)

find_package(LibAvCodec REQUIRED)
find_package(LibAvUtil REQUIRED)
find_package(LibAvFormat REQUIRED)
find_package(LibSwScale REQUIRED)
find_package(LibJpeg REQUIRED)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../../)
include_directories(${whisperlib_INCLUDE_DIR})
include_directories(${whisperstreamlib_INCLUDE_DIR})


######################################################################

include(FindPkgConfig)

include_directories(${AvCodec_INCLUDE_DIR})
include_directories(${AvUtil_INCLUDE_DIR})
include_directories(${AvFormat_INCLUDE_DIR})
include_directories(${SwScale_INCLUDE_DIR})
# Safeguard for some older ffmpeg versions
include_directories(${AvCodec_INCLUDE_DIR}/ffmpeg)

include(../../cmake/common.cmake)

add_definitions(-D __AV_CODEC_INCLUDE_FILE="<${AV_CODEC_INCLUDE_FILE}>")
add_definitions(-D __AV_UTIL_INCLUDE_FILE="<${AV_UTIL_INCLUDE_FILE}>")
add_definitions(-D __AV_FORMAT_INCLUDE_FILE="<${AV_FORMAT_INCLUDE_FILE}>")
add_definitions(-D __SW_SCALE_INCLUDE_FILE="<${SW_SCALE_INCLUDE_FILE}>")

link_libraries(${COMMON_LINK_LIBRARIES})

ADD_LIBRARY(whisper_featurelib SHARED
  decoding/bitmap_extractor.cc
  decoding/feature_extractor.cc
  decoding/feature_data.cc 
  util/jpeg_util.cc
  util/flv_thumbnailer.cc
  util/rtmp_thumbnailer.cc)

ADD_DEPENDENCIES(whisper_featurelib
  whisper_lib
  whisper_medialib
  whisper_streamlib)
TARGET_LINK_LIBRARIES(whisper_featurelib
  ${AvCodec_LIBRARIES}
  ${AvFormat_LIBRARIES}
  ${AvUtil_LIBRARIES}
  ${SwScale_LIBRARIES}
  ${LibJpeg_LIBRARIES}

  ${whisperlib_LIBRARY}
  ${whisperstreamlib_LIBRARY}
  ${COMMON_LINK_LIBRARIES})

##########

ADD_EXECUTABLE(flv_thumbnailer
  util/flv_thumbnailer_app.cc)

ADD_DEPENDENCIES(flv_thumbnailer
  whisper_lib
  whisper_streamlib
  whisper_featurelib)

TARGET_LINK_LIBRARIES(flv_thumbnailer
  ${AvCodec_LIBRARIES}
  whisper_lib
  whisper_streamlib
  whisper_featurelib)

##########

ADD_EXECUTABLE(flv_extract_features
  util/flv_extract_features.cc)

ADD_DEPENDENCIES(flv_extract_features
  whisper_lib
  whisper_streamlib
  whisper_featurelib)

TARGET_LINK_LIBRARIES(flv_extract_features
  ${AvCodec_LIBRARIES}
  whisper_lib
  whisper_streamlib
  whisper_featurelib)

##########

ADD_EXECUTABLE(flv_grep_features
  util/flv_grep_features.cc)

ADD_DEPENDENCIES(flv_grep_features
  whisper_lib
  whisper_streamlib
  whisper_featurelib)

TARGET_LINK_LIBRARIES(flv_grep_features
  ${AvCodec_LIBRARIES}
  whisper_lib
  whisper_streamlib
  whisper_featurelib)

##########

install(TARGETS whisper_featurelib
         DESTINATION lib)


INSTALL (TARGETS flv_thumbnailer
         DESTINATION bin)
INSTALL (TARGETS flv_extract_features
         DESTINATION bin)
INSTALL (TARGETS flv_grep_features
         DESTINATION bin)

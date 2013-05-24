# Copyright (c) 2012, Whispersoft s.r.l.
# All rights reserved.
#

find_package(LibAvCodec REQUIRED)
find_package(LibSwScale REQUIRED)

include_directories(${AvCodec_INCLUDE_DIR})
include_directories(${SwScale_INCLUDE_DIR})
# Safeguard for some older ffmpeg versions
include_directories(${AvCodec_INCLUDE_DIR}/ffmpeg)

add_definitions(-D __AV_CODEC_INCLUDE_FILE="<${AV_CODEC_INCLUDE_FILE}>")
add_definitions(-D __SW_SCALE_INCLUDE_FILE="<${SW_SCALE_INCLUDE_FILE}>")

SET(whisperfeature_LIBRARY "whisper_featurelib")

ADD_EXECUTABLE(thumbnailer
  thumbnailer_app.cc)

TARGET_LINK_LIBRARIES(thumbnailer
  ${AvCodec_LIBRARIES}
  ${whisperfeature_LIBRARY}
  whisper_lib
  whisper_streamlib)

install(TARGETS thumbnailer
  DESTINATION bin)

# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/auto)
SET(WHISPERPROC_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/whisperproc.rpc )
SET(WHISPERPROC_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/whisperproc_types.cc)
ADD_CUSTOM_COMMAND(
  OUTPUT ${WHISPERPROC_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${WHISPERPROC_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/auto/whisperproc_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/auto/whisperproc_invokers"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/auto/whisperproc_types.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/auto/whisperproc.py"'
  DEPENDS ${WHISPERPROC_RPC_INPUT_FILES} ${RPC_PARSER_EXE}
)

if (WITH_GSTREAMER)
  find_package(LibAvCodec REQUIRED)
  find_package(LibSwScale REQUIRED)

  include_directories(${AvCodec_INCLUDE_DIR})
  include_directories(${SwScale_INCLUDE_DIR})
  # Safeguard for some older ffmpeg versions
  include_directories(${AvCodec_INCLUDE_DIR}/ffmpeg)

  add_definitions(-D __AV_CODEC_INCLUDE_FILE="<${AV_CODEC_INCLUDE_FILE}>")
  add_definitions(-D __SW_SCALE_INCLUDE_FILE="<${SW_SCALE_INCLUDE_FILE}>")
endif (WITH_GSTREAMER)

if (WITH_GSTREAMER)
  SET(THUMBNAILER_SRC "whisperproc_thumbnails.cc")
  SET(whisperfeature_LIBRARY "whisper_featurelib")
else (WITH_GSTREAMER)
  SET(THUMBNAILER_SRC "")
  Set(whisperfeature_LIBRARY"")
  add_definitions(-D__DISABLE_FEATURES__)
endif (WITH_GSTREAMER)

ADD_EXECUTABLE(whisperproc
  whisperproc_util.cc
  ${THUMBNAILER_SRC}
  whisperproc.cc
  ${WHISPERPROC_RPC_OUTPUT_FILES})

ADD_DEPENDENCIES(whisperproc
  ${whisperfeature_LIBRARY}
  whisper_lib
  whisper_streamlib)

TARGET_LINK_LIBRARIES(whisperproc
  ${AvCodec_LIBRARIES}
  ${whisperfeature_LIBRARY}
  ${whisperlib_LIBRARY}
  ${whisperstreamlib_LIBRARY})

install(TARGETS whisperproc
  DESTINATION bin)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/whisperproc)
install(FILES
  scripts/whisperproc_detect_bad_clips.py
  scripts/whisperproc_fillup.py
  scripts/whisperproc_range_select.py
  scripts/whisperproc_util.py
  scripts/whisperproc_get_coverage_files.py
  scripts/whisperproc_cleanup_files.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/whisperproc
  PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ WORLD_EXECUTE GROUP_EXECUTE OWNER_EXECUTE)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/private/whisperproc)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/whisperproc_types.php
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/private/whisperproc)
if (WITH_ADMIN)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/private/whisperproc)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/auto/whisperproc_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/private/whisperproc)
endif (WITH_ADMIN)

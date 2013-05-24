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

ADD_EXECUTABLE(whisperproc
  processor.cc
  whisperproc.cc
  ${WHISPERPROC_RPC_OUTPUT_FILES})

TARGET_LINK_LIBRARIES(whisperproc
  whisper_lib
  whisper_streamlib)

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

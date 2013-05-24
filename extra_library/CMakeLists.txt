# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
project (whispersoft)

######################################################################

# extra_library -> MODULE

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/auto)
SET(EXTRA_LIB_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/extra_library.rpc )
SET(EXTRA_LIB_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_types.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_invokers.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_wrappers.cc)
ADD_CUSTOM_COMMAND (
  OUTPUT ${EXTRA_LIB_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${EXTRA_LIB_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_invokers" , fClientTypes="${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_types", fClientWrappers="${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_wrappers"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_types.php", fWrappers="${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_invokers.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library.py"'
  DEPENDS ${EXTRA_LIB_RPC_INPUT_FILES} ${RPC_PARSER_EXE}
  )

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/auto)
SET(AUX_ELEMENT_MAPPER_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/aux_element_mapper.rpc )
SET(AUX_ELEMENT_MAPPER_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_types.cc)

ADD_CUSTOM_COMMAND (
  OUTPUT ${AUX_ELEMENT_MAPPER_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${AUX_ELEMENT_MAPPER_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_invokers"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_types.php", fWrappers="${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_wrappers.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_types.py"'
  DEPENDS ${AUX_ELEMENT_MAPPER_RPC_INPUT_FILES} ${RPC_PARSER_EXE}
  )

####################

find_package(LibAvCodec REQUIRED)
find_package(LibSwScale REQUIRED)

include_directories(${AvCodec_INCLUDE_DIR})
include_directories(${SwScale_INCLUDE_DIR})
# Safeguard for some older ffmpeg versions
include_directories(${AvCodec_INCLUDE_DIR}/ffmpeg)

add_definitions(-D __AV_CODEC_INCLUDE_FILE="<${AV_CODEC_INCLUDE_FILE}>")
add_definitions(-D __SW_SCALE_INCLUDE_FILE="<${SW_SCALE_INCLUDE_FILE}>")

####################

# to use for your utilities..
ADD_LIBRARY (extra_streaming_elements SHARED
  extra_library.cc

  feature_detector/feature_detector.cc
  flavouring/flavouring_element.cc
  http_authorizer/http_authorizer.cc
  schedule/schedule_policy.cc
  logic_policy/logic_policy.cc
  time_delay/time_delay_policy.cc
  time_range/time_range_element.cc
  time_shift/time_shift_element.cc
  aux_element_mapper.cc

  ${EXTRA_LIB_RPC_OUTPUT_FILES}
  ${AUX_ELEMENT_MAPPER_RPC_OUTPUT_FILES}
  )

ADD_DEPENDENCIES (extra_streaming_elements
  whisper_lib
  whisper_streamlib
  whisper_featurelib)

TARGET_LINK_LIBRARIES(
  extra_streaming_elements
  ${AvCodec_LIBRARIES}
  whisper_featurelib
  whisper_lib
  whisper_streamlib)


INSTALL(TARGETS extra_streaming_elements
  DESTINATION modules)

####################

FILE(MAKE_DIRECTORY
  ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/private/extra_library)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_types.php
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_wrappers.php
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_invokers.php
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_types.php
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/private/extra_library)

if (WITH_ADMIN)
  FILE(MAKE_DIRECTORY
    ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/private/extra_library)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_types.php
    ${CMAKE_CURRENT_SOURCE_DIR}/auto/aux_element_mapper_wrappers.php
    ${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_invokers.php
    ${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/private/extra_library)
endif (WITH_ADMIN)

FILE(MAKE_DIRECTORY
  ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/private/extra_library)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/extra_library.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/private/extra_library)

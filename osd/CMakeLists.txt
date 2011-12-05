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

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/base/auto)

SET(OSD_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/base/osd.rpc )
SET(OSD_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_types.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_invokers.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_client_wrappers.cc)
ADD_CUSTOM_COMMAND(
  OUTPUT ${OSD_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${OSD_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_invokers", fClientTypes="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_types", fClientWrappers="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_client_wrappers"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_types.php", fWrappers="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_invokers.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd.py"'
  DEPENDS ${OSD_RPC_INPUT_FILES} ${RPC_PARSER_TARGET}
  )


######################################################################

# osd_library -> MODULE

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto)
SET(OSD_LIB_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/osd_library/osd_library.rpc )
SET(OSD_LIB_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_types.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_invokers.cc)
ADD_CUSTOM_COMMAND (
  OUTPUT ${OSD_LIB_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${OSD_LIB_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_invokers"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_types.php", fWrappers="${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_invokers.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library.py"'
  DEPENDS ${OSD_LIB_RPC_INPUT_FILES} ${RPC_PARSER_TARGET}
  )

####################

# to use for your utilities..
ADD_LIBRARY (osd_library SHARED
  base/osd_tag.cc
  base/osd_to_flv_encoder.cc
  base/flv_to_osd_decoder.cc
  ${OSD_RPC_OUTPUT_FILES}
  ${OSD_LIB_RPC_OUTPUT_FILES}
  )

ADD_DEPENDENCIES (osd_library
  whisper_lib
  whisper_streamlib)

TARGET_LINK_LIBRARIES(osd_library
  ${whisperlib_LIBRARY}
  ${whisperstreamlib_LIBRARY})

INSTALL(TARGETS osd_library
  DESTINATION lib)

####################

ADD_LIBRARY (osd_streaming_elements MODULE
  base/osd_tag.cc
  base/osd_to_flv_encoder.cc
  base/flv_to_osd_decoder.cc

  osd_library/osd_library.cc
  osd_library/osd_decoder/osd_decoder_element.cc
  osd_library/osd_encoder/osd_encoder_element.cc
  osd_library/osd_injector/osd_injector_element.cc
  osd_library/osd_associator/osd_associator_element.cc
  osd_library/osd_remote_associator/osd_associator_client_element.cc
  osd_library/osd_state_keeper/osd_state_keeper_element.cc

  ${OSD_RPC_OUTPUT_FILES}
  ${OSD_LIB_RPC_OUTPUT_FILES}
  )

ADD_DEPENDENCIES (osd_streaming_elements
  whisper_lib
  whisper_streamlib)

INSTALL (TARGETS osd_streaming_elements
  DESTINATION modules)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/private/osd/base)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_invokers.php
  ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_types.php
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/private/osd/base)
FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/private/osd/osd_library)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_invokers.php
  ${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_types.php
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/private/osd/osd_library)

if (WITH_ADMIN)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/private/osd/base)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_invokers.php
    ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/private/osd/base)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/private/osd/osd_library)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_invokers.php
    ${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/private/osd/osd_library)
endif (WITH_ADMIN)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/private/osd/base)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/osd.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/private/osd/base)
FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/private/osd/osd_library)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/osd_library/auto/osd_library.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/private/osd/osd_library)

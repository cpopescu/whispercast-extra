project (whispersoft)

SET(RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/state.rpc)
SET(RPC_OUTPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/auto/types.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/auto/types.cc
                     ${CMAKE_CURRENT_SOURCE_DIR}/auto/server_invokers.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/auto/server_invokers.cc)

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/auto)

ADD_CUSTOM_COMMAND (
  DEPENDS ${RPC_INPUT_FILES} ${RPC_PARSER_EXE}
  OUTPUT  ${RPC_OUTPUT_FILES}
  PRE_BUILD
  COMMAND ${RPC_PARSER_EXE} ${RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/auto/types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/auto/server_invokers"'
  )

ADD_EXECUTABLE(transslave
  ${RPC_OUTPUT_FILES}
  master_wrapper.cc
  rpc_slave_manager_service.cc
  slave.cc)

ADD_DEPENDENCIES(transslave
  media_manager_common
  whisper_lib)
TARGET_LINK_LIBRARIES(transslave
  media_manager_common
  whisper_lib)

INSTALL (TARGETS transslave
         DESTINATION bin)

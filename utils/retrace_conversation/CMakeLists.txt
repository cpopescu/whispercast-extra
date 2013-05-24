project (whispersoft)

ADD_EXECUTABLE(retrace_conversation
  retrace_conversation.cc)
ADD_DEPENDENCIES(retrace_conversation
  whisper_lib)
TARGET_LINK_LIBRARIES(retrace_conversation
  whisper_lib)

project (whispersoft)

ADD_EXECUTABLE(rtmp_decode_trace
  rtmp_decode_trace.cc)
ADD_DEPENDENCIES(rtmp_decode_trace
  whisper_streamlib
  whisper_lib)
TARGET_LINK_LIBRARIES(rtmp_decode_trace
  whisper_lib
  whisper_streamlib)

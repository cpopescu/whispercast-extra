project (whispersoft)

ADD_EXECUTABLE(rtmp_proxy
  rtmp_proxy.cc)
ADD_DEPENDENCIES(rtmp_proxy
  whisper_streamlib
  whisper_lib)
TARGET_LINK_LIBRARIES(rtmp_proxy
  whisper_lib
  whisper_streamlib)

project (whispersoft)

ADD_EXECUTABLE(audio2flv
  audio2flv.cc)
ADD_DEPENDENCIES(audio2flv
  whisper_streamlib
  whisper_lib)
TARGET_LINK_LIBRARIES(audio2flv
  whisper_lib
  whisper_streamlib)

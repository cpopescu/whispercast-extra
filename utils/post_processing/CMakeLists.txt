project (whispersoft)

ADD_EXECUTABLE(commercial_process
  commercial_process.cc)
ADD_DEPENDENCIES(commercial_process
  osd_library
  whisper_streamlib
  whisper_lib)
TARGET_LINK_LIBRARIES(commercial_process
  osd_library
  ${whisperstreamlib_LIBRARY}
  ${whisperlib_LIBRARY})

project (whispersoft)

find_package(MySQLpp REQUIRED)

ADD_EXECUTABLE(stats_post_processor
  mysql_stats_saver.cc
  mysql_stats_saver_complete.cc
  mysql_stats_saver_light.cc
  mysql_stats_saver_custom.cc
  stats_post_processor.cc)
ADD_DEPENDENCIES(stats_post_processor
  whisper_lib)
if (WITH_GSTREAMER)
ADD_DEPENDENCIES(stats_post_processor
  whisper_streamlib)
endif (WITH_GSTREAMER)
TARGET_LINK_LIBRARIES(stats_post_processor
  ${MySQLpp_LIBRARIES}
  ${whisperlib_LIBRARY}
  ${whisperstreamlib_LIBRARY})

install(TARGETS stats_post_processor
  DESTINATION bin)

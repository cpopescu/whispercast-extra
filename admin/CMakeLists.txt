# Main cmake configuration file - we set a bunch of things here ..
#
# To build the project(s), run in this directory:
#    cmake .
#    make
#
# To build in Debug mode:
#    cmake . -DCMAKE_BUILD_TYPE=Debug
#    make
#
# Specify the install dir with:
#    cmake -DINSTALL_PREFIX=<prefix_dir> ...
#
# You can continue with testing:
#    make test
# And installation of the binaries:
#    make install
#
######################################################################

project (whispersoft)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_MICRO "1")
set(VERSION_PATCH "0")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Administration interface")

cmake_minimum_required(VERSION 2.4)

IF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)
  CMAKE_POLICY(SET CMP0002 NEW)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)

######################################################################

install(FILES ../../scripts/shflags
  DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/install"
  PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES install/install
  DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/install"
  PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES
  install/database.sql
  install/application.ini
  DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/install")

install(DIRECTORY application DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/application/configs")
install(DIRECTORY library/PHPTAL-1.2.1/ DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/library/PHPTAL")
install(DIRECTORY library/ZendFramework-1.10.3-minimal/library/Zend/ DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/library/Zend")
install(FILES
  library/PHPTAL_Zend_View.php
  library/PHPTAL_Zend_View_Translator.php
  DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/library")
  
install(DIRECTORY public/css DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/public")
install(DIRECTORY public/images DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/public")
install(DIRECTORY public/js DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/public")
install(DIRECTORY public/player DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/public")
install(DIRECTORY public/swf DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/public")
install(DIRECTORY public/yui/2.9.0/build/ DESTINATION "${CMAKE_INSTALL_PREFIX_ADMIN}/public/yui")

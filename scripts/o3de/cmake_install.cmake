# Install script for directory: C:/Users/lesaelr/o3de/scripts/o3de

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/lesaelr/o3de/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/lesaelr/o3de/scripts/o3de/tests/cmake_install.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xCorex" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/scripts" TYPE PROGRAM MESSAGE_NEVER FILES "C:/Users/lesaelr/o3de/scripts/o3de/../o3de.bat")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xCorex" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/scripts" TYPE FILE MESSAGE_NEVER FILES "C:/Users/lesaelr/o3de/scripts/o3de/../o3de.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xCorex" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/scripts/o3de" TYPE PROGRAM MESSAGE_NEVER FILES "C:/Users/lesaelr/o3de/scripts/o3de/setup.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xCorex" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/scripts/o3de" TYPE FILE MESSAGE_NEVER FILES "C:/Users/lesaelr/o3de/scripts/o3de/README.txt")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xCorex" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/scripts/o3de" TYPE DIRECTORY MESSAGE_NEVER FILES "C:/Users/lesaelr/o3de/scripts/o3de/." REGEX "/tests$" EXCLUDE REGEX "/platform$" EXCLUDE REGEX "/cmakelists\\.txt$" EXCLUDE REGEX "/[^/]*\\.cmake$" EXCLUDE REGEX "/\\_\\_pycache\\_\\_$" EXCLUDE REGEX "/[^/]*\\.egg\\-info$" EXCLUDE)
endif()


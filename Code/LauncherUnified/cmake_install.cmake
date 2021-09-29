# Install script for directory: C:/Users/lesaelr/o3de/Code/LauncherUnified

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

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xCorex" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/LauncherGenerator" TYPE FILE MESSAGE_NEVER FILES
    "C:/Users/lesaelr/o3de/Code/LauncherUnified/launcher_generator.cmake"
    "C:/Users/lesaelr/o3de/Code/LauncherUnified/launcher_project_files.cmake"
    "C:/Users/lesaelr/o3de/Code/LauncherUnified/LauncherProject.cpp"
    "C:/Users/lesaelr/o3de/Code/LauncherUnified/StaticModules.in"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xCorex" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/LauncherGenerator/Platform" TYPE DIRECTORY MESSAGE_NEVER FILES "C:/Users/lesaelr/o3de/Code/LauncherUnified/Platform/Windows")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xCorex" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/LauncherGenerator/Platform" TYPE DIRECTORY MESSAGE_NEVER FILES "C:/Users/lesaelr/o3de/Code/LauncherUnified/Platform/Common")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xCorex" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE MESSAGE_NEVER FILES "C:/Users/lesaelr/o3de/Code/LauncherUnified/FindLauncherGenerator.cmake")
endif()


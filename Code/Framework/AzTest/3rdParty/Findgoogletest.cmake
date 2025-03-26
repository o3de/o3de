#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# GoogleTest and GoogleMock used to be two different repositories, but they are now combined into one repository.
# This script fetches the combined repository and builds both libraries.

if (TARGET 3rdParty::googletest::GTest)
    return()
endif()

if (TARGET 3rdParty::googletest::GMock)
    return()
endif()

# You should not be generating dependencies on googletest (via AzTest)
# if you are on a platform that cannot actually compile googletest (See cmake/Platform/platformname/PAL_platformname.cmake)
if (NOT PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED)
    return()
endif()

set(GOOGLETEST_GIT_REPOSITORY "https://github.com/google/googletest.git")
set(GOOGLETEST_GIT_TAG b514bdc898e2951020cbdca1304b75f5950d1f59) # tag name is v1.15.2
set(GOOGLETEST_VERSION_STRING "v1.15.2")

message(STATUS "AzTest uses googletest ${GOOGLETEST_VERSION_STRING} (BSD-3-Clause) from ${GOOGLETEST_GIT_REPOSITORY}")

# there are optional flags you can pass FetchContent that make the project cleaner but require newer versions of cmake

set(ADDITIONAL_FETCHCONTENT_FLAGS "")

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.25")
    list(APPEND ADDITIONAL_FETCHCONTENT_FLAGS "SYSTEM")  # treat it as a 3p library, that is, do not issue warnings using the same warning level
endif()

set(CMAKE_POLICY_DEFAULT_CMP0148 OLD)
set(OLD_CMAKE_WARN_DEPRECATED ${CMAKE_WARN_DEPRECATED})
set(CMAKE_WARN_DEPRECATED FALSE CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY ${GOOGLETEST_GIT_REPOSITORY}
    GIT_TAG ${GOOGLETEST_GIT_TAG}
    ${ADDITIONAL_FETCHCONTENT_FLAGS}
    GIT_SHALLOW TRUE
)

# CMAKE_ARGS don't actually affect FetchContent (currently by design) https://gitlab.kitware.com/cmake/cmake/-/issues/20799
set(PYTHONINTERP_FOUND ON)
set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
set(PYTHON_VERSION_STRING ${Python_VERSION})
set(gtest_force_shared_crt ON)
set(BUILD_GMOCK ON)
set(BUILD_GTEST ON)
set(INSTALL_GTEST OFF)
set(gmock_build_tests OFF)
set(gtest_build_tests OFF)
set(gtest_build_samples OFF)
set(gtest_hide_internal_symbols ON)

# Save values that apply globally that the 3rd party library may mess with
# Some of these are null, hence the set(xxx "quoted value") so that if it isn't set it becomes the empty string.

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(OLD_CMAKE_CXX_VISIBILITY_PRESET "${CMAKE_CXX_VISIBILITY_PRESET}")
set(OLD_CMAKE_VISIBILITY_INLINES_HIDDEN "${CMAKE_VISIBILITY_INLINES_HIDDEN}")

FetchContent_MakeAvailable(googletest)

set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)
if (NOT "${OLD_CMAKE_CXX_VISIBILITY_PRESET}" STREQUAL "")
    set(CMAKE_CXX_VISIBILITY_PRESET "${OLD_CMAKE_CXX_VISIBILITY_PRESET}")
else()
    unset(CMAKE_CXX_VISIBILITY_PRESET)
endif()
if (NOT "${OLD_CMAKE_VISIBILITY_INLINES_HIDDEN}" STREQUAL "")
    set(CMAKE_VISIBILITY_INLINES_HIDDEN "${OLD_CMAKE_VISIBILITY_INLINES_HIDDEN}")
else()
    unset(CMAKE_VISIBILITY_INLINES_HIDDEN)
endif()

# Let's not clutter the root of any IDE folder structure with 3rd party dependencies
# Setting the FOLDER makes it show up there in the solution build in VS and similarly
# any other IDEs that organize in folders.
set_target_properties(
        gtest 
        gmock 
        gtest_main 
        gmock_main 
    PROPERTIES 
        FOLDER "3rdParty Dependencies")

unset(CMAKE_POLICY_DEFAULT_CMP0148)
set(CMAKE_WARN_DEPRECATED ${OLD_CMAKE_WARN_DEPRECATED} CACHE BOOL "" FORCE)


FetchContent_GetProperties(
    googletest
    SOURCE_DIR googletest_source_dir)
    
# GoogleTest and GoogleMock are considered public API - accessed by adding a dependency to AZ::AzTest.
# This means we DO need to ship the include folders, in the installer and we do need the static libs, in case you link to AzTest.
# By Default, no headers or includes are actually shipped, so we need to explicitly add them using ly_install.
# doing so also helps us split them up between debug and non-debug versions

# install the headers
# note that source files include these headers using the name of the library, for example, they do
# #include <gtest/gtest.h> and #include <gmock/gmock.h>, not just <gmock.h> or <gtest.h>
# One option would be to just put these folders in (root)/include/gtest and (root)/include/gmock
# and then add (root)/include as the include path for targets using these libraries.
# However, that would cause any OTHER files in (root)/include to be visible to the compiler when all
# you asked for is gmock/gtest.  So to avoid this, we instead copy the include files into
# install/gmock_gtest/gmock 
# install/gmock_gtest/gtest
# and have install/gmock_gtest be the include path that is added to your compiler settings when you use these libraries, 
# so that #include <gtest/gtest> still works (since its relative to that path) BUT it adds no additional other
# files or libraries to your include path that would otherwise leak in.
ly_install(DIRECTORY ${googletest_source_dir}/googletest/include/gtest DESTINATION include/gmock_gtest COMPONENT CORE)
ly_install(DIRECTORY ${googletest_source_dir}/googlemock/include/gmock DESTINATION include/gmock_gtest COMPONENT CORE)
# include the license files just for good measure (Although using the library will disclose the
# source git repository and version anyway.
ly_install(FILES ${googletest_source_dir}/LICENSE COMPONENT CORE DESTINATION include/gmock_gtest/gtest)
ly_install(FILES ${googletest_source_dir}/LICENSE COMPONENT CORE DESTINATION include/gmock_gtest/gmock)

# Make the installer find-files be used when in an installer pre-built mode
ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findgoogletest.cmake DESTINATION cmake/3rdParty)

# install the libraries making sure to use different directories for debug/release/etc
set(BASE_LIBRARY_FOLDER "lib/${PAL_PLATFORM_NAME}")
foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${conf} UCONF)
    ly_install(TARGETS gtest gmock
        ARCHIVE
            DESTINATION "${BASE_LIBRARY_FOLDER}/${conf}"
            COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
            CONFIGURATIONS ${conf}
    )
endforeach()

# do not use ly_create_alias as that will autogenerate duplicate calls inside the installer.
# ly_create_alias would create "GTest", "GMock" and "GTest::GTest" and "GMock::GMock" aliases, as well
# as add them to an internal list of targets to autogenerate imported targets for.

# That's okay if you're going to let the installer do its own thing, but in our case, we're including 
# a FindXXXX in the installer, so create them directly, without calling the LY macro
add_library(3rdParty::googletest::GTest ALIAS gtest)
add_library(3rdParty::googletest::GMock ALIAS gmock)

set(googletest_FOUND TRUE)



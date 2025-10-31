#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

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


# This file is run when in pre-built installer mode, and thus GTest and GMock are pre-built static libraries
# ALWAYS DISCLOSE THE USE OF 3rd Party Libraries.  Even if they are internally linked.
message(STATUS "AzTest library uses googletest v1.15.2 (BSD-3-Clause) from https://github.com/google/googletest.git")

set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")

add_library(gtest STATIC IMPORTED GLOBAL)
set_target_properties(gtest PROPERTIES 
    IMPORTED_LOCATION       "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_DEBUG "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}")
ly_target_include_system_directories(TARGET gtest INTERFACE "${LY_ROOT_FOLDER}/include/gmock_gtest")

add_library(gmock STATIC IMPORTED GLOBAL)
set_target_properties(gmock PROPERTIES 
    IMPORTED_LOCATION       "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_DEBUG "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX}")
ly_target_include_system_directories(TARGET gmock INTERFACE "${LY_ROOT_FOLDER}/include/gmock_gtest")
    
add_library(3rdParty::googletest::GTest ALIAS gtest)
add_library(3rdParty::googletest::GMock ALIAS gmock)

set(googletest_FOUND TRUE)

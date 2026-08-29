#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Installer-mode counterpart of Code/Framework/AzTest/3rdParty/Findgoogletest.cmake
# The libraries are already built here, so just import them.

if(TARGET 3rdParty::googletest::GTest OR TARGET 3rdParty::googletest::GMock)
    return()
endif()

# Do not depend on googletest (via AzTest) where it cannot compile.
# See cmake/Platform/<platform>/PAL_<platform>.cmake
if(NOT PAL_TRAIT_TEST_GOOGLE_TEST_SUPPORTED)
    return()
endif()

set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")

foreach(targetname gtest gmock)
    add_library(${targetname} STATIC IMPORTED GLOBAL)
    set_target_properties(${targetname} PROPERTIES
        IMPORTED_LOCATION "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}${targetname}${CMAKE_STATIC_LIBRARY_SUFFIX}"
        IMPORTED_LOCATION_DEBUG "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}${targetname}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
    # Both include their headers as <gtest/...> and <gmock/...>, relative to this one folder.
    ly_target_include_system_directories(TARGET ${targetname} INTERFACE "${LY_ROOT_FOLDER}/include/gmock_gtest")
endforeach()

add_library(3rdParty::googletest::GTest ALIAS gtest)
add_library(3rdParty::googletest::GMock ALIAS gmock)

set(googletest_FOUND TRUE)

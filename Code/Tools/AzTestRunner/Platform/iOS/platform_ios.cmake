#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(TEST_LAUNCHER_BUILD_DEPENDENCIES AzCore AzTest)

set(LY_LINK_OPTIONS
    PRIVATE
        -ObjC
)

set(ly_game_resource_folder ${CMAKE_CURRENT_LIST_DIR}/Resources)
if (NOT EXISTS ${ly_game_resource_folder})
    message(FATAL_ERROR "Missing expected resources folder")
endif()

target_sources(AzTestRunner PRIVATE ${ly_game_resource_folder}/Images.xcassets)
set_target_properties(AzTestRunner PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${ly_game_resource_folder}/Info.plist
    RESOURCE ${ly_game_resource_folder}/Images.xcassets
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AzTestRunnerAppIcon
)

find_package(XCTest REQUIRED)

xctest_add_bundle(TestLauncherTarget AzTestRunner
    ${ly_game_resource_folder}/Info.plist
    ${CMAKE_CURRENT_LIST_DIR}/TestLauncherTarget.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)


# XCTest adds the test as a target, but we still need to set dependencies.
set(include_directories "${CMAKE_CURRENT_SOURCE_DIR}/src")
foreach(dependency ${TEST_LAUNCHER_BUILD_DEPENDENCIES})
    get_target_property(includes ${dependency} INCLUDE_DIRECTORIES)
    LIST(APPEND include_directories ${includes})
    target_link_libraries(TestLauncherTarget PRIVATE ${dependency})
endforeach()

set_property(TARGET TestLauncherTarget PROPERTY INCLUDE_DIRECTORIES ${include_directories})

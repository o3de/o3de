#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    return()
endif()

ly_add_pytest(
    NAME o3de_export_source_built_project
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_export_source_built_project.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_export_source_ios_xcode
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_export_source_ios_xcode.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_export_source_android
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_export_source_android.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    return()
endif()

# Add a test to test out the o3de package `o3de.py register` command
ly_add_pytest(
    NAME o3de_register
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit_test_register.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_cmake
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit_test_cmake.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_global_project
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit_test_global_project.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_manifest
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit_test_manifest.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_template
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit_test_engine_template.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

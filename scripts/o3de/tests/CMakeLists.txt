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

# Add a test to test out the o3de package `o3de.py register` command
ly_add_pytest(
    NAME o3de_register
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_register.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_cmake
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_cmake.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_disable_gem
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_disable_gem.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_download
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_download.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_enable_gem
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_enable_gem.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_global_project
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_global_project.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_manifest
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_manifest.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_engine_properties
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_engine_properties.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_project_properties
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_project_properties.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_repo
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_repo.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_gem_properties
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_gem_properties.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_template
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_engine_template.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_register_show
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_print_registration.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_export_project
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_export_project.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

ly_add_pytest(
    NAME o3de_utils
    PATH ${CMAKE_CURRENT_LIST_DIR}/test_utils.py
    TEST_SUITE smoke
    EXCLUDE_TEST_RUN_TARGET_FROM_IDE
)

add_subdirectory(ExportScripts)

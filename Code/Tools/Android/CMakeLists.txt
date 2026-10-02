#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_install_directory(
    DIRECTORIES
        ProjectBuilder
        ProjectGenerator
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_pytest(
        NAME android_project_generator
        PATH ${CMAKE_CURRENT_LIST_DIR}/ProjectGenerator/tests
        TEST_SUITE smoke
        EXCLUDE_TEST_RUN_TARGET_FROM_IDE
    )
endif()

#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

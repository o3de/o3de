#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # for PAL_TRAIT_WHITEBOX Traits

if(PAL_TRAIT_WHITEBOX_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_pytest(
        NAME AutomatedTesting::WhiteBoxTests
        TEST_SUITE main
        TEST_SERIAL
        PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Main.py
        RUNTIME_DEPENDENCIES
            Legacy::Editor
            AZ::AssetProcessor
            AutomatedTesting.Assets
        COMPONENT
            WhiteBox
        LABELS REQUIRES_tiaf
    )
endif()

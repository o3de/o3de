#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT PAL_TRAIT_BUILD_TESTS_SUPPORTED OR NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_pytest(
    NAME AutomatedTesting::EditorLevelLoadingPerfTests_DX12
    TEST_SUITE periodic
    TEST_REQUIRES gpu
    TEST_SERIAL
    PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Periodic_DX12.py
    RUNTIME_DEPENDENCIES
        Legacy::Editor
        AZ::AssetProcessor
        AutomatedTesting.Assets
    COMPONENT
        Performance
)

ly_add_pytest(
    NAME AutomatedTesting::EditorLevelLoadingPerfTests_Vulkan
    TEST_SUITE periodic
    TEST_REQUIRES gpu
    TEST_SERIAL
    PATH ${CMAKE_CURRENT_LIST_DIR}/TestSuite_Periodic_Vulkan.py
    RUNTIME_DEPENDENCIES
        Legacy::Editor
        AZ::AssetProcessor
        AutomatedTesting.Assets
    COMPONENT
        Performance
)

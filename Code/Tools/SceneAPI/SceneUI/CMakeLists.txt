#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME SceneUI SHARED
    NAMESPACE AZ
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        SceneUI_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
        PUBLIC
            ../..
    COMPILE_DEFINITIONS
        PRIVATE
            SCENE_UI_EXPORTS
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzToolsFramework
            AZ::SceneCore
            AZ::SceneData
        PUBLIC
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Widgets
)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME SceneUI.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            SceneUI_testing_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::SceneUI
    )
    ly_add_googletest(
        NAME AZ::SceneUI.Tests
        LABELS REQUIRES_tiaf
    )
endif()

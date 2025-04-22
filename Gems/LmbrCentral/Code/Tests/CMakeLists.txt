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
    
ly_add_target(
    NAME LmbrCentral.Mocks HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        lmbrcentral_mocks_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            ../Mocks
)
    
ly_add_target(
    NAME LmbrCentral.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        lmbrcentral_tests_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ../Source
            .
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzTest
            AZ::AzTestShared
            Legacy::CryCommon
            AZ::AzFramework
            Gem::LmbrCentral.Static
            Gem::LmbrCentral.Mocks
)
ly_add_googletest(
    NAME Gem::LmbrCentral.Tests
    LABELS REQUIRES_tiaf
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME LmbrCentral.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            lmbrcentral_editor_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                ..
                ../Source
                .
        COMPILE_DEFINITIONS
            PRIVATE
                LMBR_CENTRAL_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                Legacy::CryCommon
                Legacy::Editor.Headers
                AZ::AzTest
                AZ::AzCore
                AZ::AzTestShared
                AZ::AzToolsFramework
                AZ::AzToolsFrameworkTestCommon
                AZ::AssetBuilderSDK
                AZ::AzManipulatorTestFramework.Static
                Gem::LmbrCentral.Static
                Gem::LmbrCentral.Editor.Static
    )
    ly_add_googletest(
        NAME Gem::LmbrCentral.Editor.Tests
        LABELS REQUIRES_tiaf
    )
endif()


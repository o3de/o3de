#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME AssetValidation.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        assetvalidation_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Legacy::CryCommon
)

ly_add_target(
    NAME AssetValidation ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        assetvalidation_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AssetValidation.Static
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME AssetValidation.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            assetvalidation_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::AssetValidation.Static
                AZ::AzFramework
                AZ::AzFrameworkTestShared
    )
    ly_add_googletest(
        NAME Gem::AssetValidation.Tests
    )
endif()

# AssetValidation should be active in all clients plus tools
ly_create_alias(NAME AssetValidation.Clients NAMESPACE Gem TARGETS Gem::AssetValidation)
ly_create_alias(NAME AssetValidation.Tools   NAMESPACE Gem TARGETS Gem::AssetValidation)


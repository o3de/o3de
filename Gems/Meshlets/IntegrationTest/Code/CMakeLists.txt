#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.IntegrationTest ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            meshlets_integration_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                Gem::Meshlets.Static
                Gem::Meshlets.Reflect
                Gem::Meshlets.Builders.Static
        RUNTIME_DEPENDENCIES
                Gem::Meshlets
                Gem::Meshlets.Builders
    )

    ly_add_googletest(NAME Gem::${gem_name}.IntegrationTest)
endif()

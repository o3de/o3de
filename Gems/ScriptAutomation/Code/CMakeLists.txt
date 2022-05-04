#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ScriptAutomation.API HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        scriptautomation_header_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
)

ly_add_target(
    NAME ScriptAutomation.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptautomation_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::ScriptAutomation.API
)

ly_add_target(
    NAME ScriptAutomation ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        scriptautomation_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::ScriptAutomation.Static
)

ly_create_alias(NAME ScriptAutomation.Servers NAMESPACE Gem TARGETS Gem::ScriptAutomation)
ly_create_alias(NAME ScriptAutomation.Builders NAMESPACE Gem TARGETS Gem::ScriptAutomation)
ly_create_alias(NAME ScriptAutomation.Clients NAMESPACE Gem TARGETS Gem::ScriptAutomation)
ly_create_alias(NAME ScriptAutomation.Tools NAMESPACE Gem TARGETS Gem::ScriptAutomation)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ScriptAutomation.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            scriptautomation_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                Gem::ScriptAutomation.Static
        RUNTIME_DEPENDENCIES
            Gem::ScriptAutomation
    )

    ly_add_googletest(
        NAME Gem::ScriptAutomation.Tests
    )
endif()

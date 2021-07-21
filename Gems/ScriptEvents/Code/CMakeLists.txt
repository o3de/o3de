#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME ScriptEvents.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptevents_common_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME ScriptEvents ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        scriptevents_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::ScriptEvents.Static
)

# the above module is for use in clients and servers
ly_create_alias(NAME ScriptEvents.Clients NAMESPACE Gem TARGETS Gem::ScriptEvents)
ly_create_alias(NAME ScriptEvents.Servers NAMESPACE Gem TARGETS Gem::ScriptEvents)


if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ScriptEvents.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            scriptevents_editor_files.cmake
            scriptevents_editor_builder_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                SCRIPTEVENTS_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AssetBuilderSDK
                Gem::ScriptEvents.Static
    )

    # the above module is for use in dev tools.
    ly_create_alias(NAME ScriptEvents.Tools    NAMESPACE Gem TARGETS Gem::ScriptEvents.Editor)
    ly_create_alias(NAME ScriptEvents.Builders NAMESPACE Gem TARGETS Gem::ScriptEvents.Editor)
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ScriptEvents.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            scriptevents_files.cmake
            scriptevents_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Include/ScriptEvents
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                Gem::ScriptEvents.Static
    )
    ly_add_googletest(
        NAME Gem::ScriptEvents.Tests
    )
endif()

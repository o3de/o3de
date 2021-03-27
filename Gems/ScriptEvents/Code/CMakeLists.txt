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

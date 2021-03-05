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
    NAME Visibility.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        visibility_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
)

ly_add_target(
    NAME Visibility ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Visibility.3b4ab3f54c2749328934c5b864355a61.v0.1.0
    FILES_CMAKE
        visibility_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Visibility.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Visibility.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            visibility_editor_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                VISIBILITY_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Legacy::Editor.Headers
                Legacy::EditorCore
                AZ::AzToolsFramework
                Gem::Visibility.Static
    )

    ly_add_target(
        NAME Visibility.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Visibility.Editor.3b4ab3f54c2749328934c5b864355a61.v0.1.0
        FILES_CMAKE
            visibility_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::Visibility.Editor.Static
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME Visibility.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                visibility_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::Visibility.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::Visibility.Editor.Tests
        )
    endif()
endif()

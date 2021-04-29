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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) #for PAL_TRAIT_NVCLOTH Traits

if(PAL_TRAIT_NVCLOTH_USE_STUB)
    include(nvcloth_stub.cmake)
    return()
endif()

ly_add_target(
    NAME NvCloth.Static STATIC
    NAMESPACE Gem
    OUTPUT_NAME Gem.NvCloth.Static
    FILES_CMAKE
        nvcloth_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::NvCloth
            # CryCommon required for 'gEnv->IsDedicated()'.
            # Because of this the module will need CrySystemEventBus to initialize gEnv
            # and tests targets will need to fake gEnv. To be removed when there is
            # an AZ replacement for asking if the game is running on a server or not.
            Legacy::CryCommon
            Gem::AtomLyIntegration_CommonFeatures.Public
        PRIVATE
            Gem::EMotionFXStaticLib
)

ly_add_target(
    NAME NvCloth ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.NvCloth
    FILES_CMAKE
        nvcloth_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
        PRIVATE
            Gem::NvCloth.Static
    RUNTIME_DEPENDENCIES
        Gem::AtomLyIntegration_CommonFeatures
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME NvCloth.Editor.Static STATIC
        NAMESPACE Gem
        OUTPUT_NAME Gem.NvCloth.Editor.Static
        AUTOMOC
        FILES_CMAKE
            nvcloth_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Include
                Source
        COMPILE_DEFINITIONS
            PRIVATE
                NVCLOTH_EDITOR
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::NvCloth.Static
                AZ::AzToolsFramework
                AZ::SceneCore
    )

    ly_add_target(
        NAME NvCloth.Editor GEM_MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.NvCloth.Editor
        FILES_CMAKE
            nvcloth_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        COMPILE_DEFINITIONS
            PRIVATE
                NVCLOTH_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::NvCloth.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::AtomLyIntegration_CommonFeatures.Editor
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME NvCloth.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        OUTPUT_NAME Gem.NvCloth.Tests
        FILES_CMAKE
            nvcloth_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTestShared
                AZ::AzTest
                Gem::NvCloth.Static
                Gem::EMotionFXStaticLib
                Gem::EMotionFX.Tests.Static
        RUNTIME_DEPENDENCIES
            Gem::EMotionFX
    )
    ly_add_googletest(
        NAME Gem::NvCloth.Tests
    )
    
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME NvCloth.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            OUTPUT_NAME Gem.NvCloth.Editor.Tests
            FILES_CMAKE
                nvcloth_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Include
                    Tests
                    Source
            COMPILE_DEFINITIONS
                PRIVATE
                    NVCLOTH_EDITOR
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTestShared
                    AZ::AzTest
                    AZ::AzToolsFrameworkTestCommon
                    Gem::NvCloth.Editor.Static
                    Gem::EMotionFX.Editor.Static
                    Gem::EMotionFX.Tests.Static
            RUNTIME_DEPENDENCIES
                Gem::EMotionFX.Editor
        )
        ly_add_googletest(
            NAME Gem::NvCloth.Editor.Tests
        )
    endif()
endif()

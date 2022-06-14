#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) #for PAL_TRAIT_NVCLOTH Traits

if(PAL_TRAIT_NVCLOTH_USE_STUB)
    include(nvcloth_stub.cmake)
    return()
endif()

ly_add_target(
    NAME NvCloth.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        nvcloth_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::NvCloth
            AZ::AzFramework
            Gem::AtomLyIntegration_CommonFeatures.Public
        PRIVATE
            Gem::EMotionFXStaticLib
)

ly_add_target(
    NAME NvCloth ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME NvCloth.Gem
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

# use the NvCloth module in clients and servers:
ly_create_alias(NAME NvCloth.Clients NAMESPACE Gem TARGETS Gem::NvCloth)
ly_create_alias(NAME NvCloth.Servers NAMESPACE Gem TARGETS Gem::NvCloth)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME NvCloth.Editor.Static STATIC
        NAMESPACE Gem
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
        OUTPUT_NAME NvCloth.Editor.Gem
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

    # use the NvCloth.Editor module in dev tools:
    ly_create_alias(NAME NvCloth.Builders NAMESPACE Gem TARGETS Gem::NvCloth.Editor)
    ly_create_alias(NAME NvCloth.Tools    NAMESPACE Gem TARGETS Gem::NvCloth.Editor)
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME NvCloth.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        OUTPUT_NAME NvCloth.Tests.Gem
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
            OUTPUT_NAME NvCloth.Editor.Tests.Gem
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

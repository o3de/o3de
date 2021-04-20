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
    FILES_CMAKE
        nvcloth_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::NvCloth
            Gem::LmbrCentral
            Gem::AtomLyIntegration_CommonFeatures.Static
        PRIVATE
            Gem::EMotionFXStaticLib
)

ly_add_target(
    NAME NvCloth ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
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
            Legacy::CryCommon
            Gem::NvCloth.Static
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

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
            PRIVATE
                Gem::EMotionFXStaticLib
    )

    ly_add_target(
        NAME NvCloth.Editor GEM_MODULE

        NAMESPACE Gem
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
                Legacy::CryCommon
                Gem::NvCloth.Editor.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME NvCloth.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            nvcloth_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::CryCommon
                AZ::AzTestShared
                AZ::AzTest
                Gem::NvCloth.Static
                Gem::EMotionFXStaticLib
                Gem::EMotionFX.Tests.Static
        RUNTIME_DEPENDENCIES
            Gem::EMotionFX
            Gem::LmbrCentral
    )
    ly_add_googletest(
        NAME Gem::NvCloth.Tests
    )
    
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME NvCloth.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
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
                    Legacy::CryCommon
                    AZ::AzTestShared
                    AZ::AzTest
                    AZ::AzToolsFrameworkTestCommon
                    Gem::NvCloth.Editor.Static
                    Gem::EMotionFX.Editor.Static
                    Gem::EMotionFX.Tests.Static
            RUNTIME_DEPENDENCIES
                Gem::EMotionFX.Editor
                Gem::LmbrCentral.Editor
        )
        ly_add_googletest(
            NAME Gem::NvCloth.Editor.Tests
        )
    endif()
endif()

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
    NAME Atom_RPI.Public STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atom_rpi_reflect_files.cmake
        atom_rpi_public_files.cmake
        ../Assets/atom_rpi_asset_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AtomCore
            AZ::AzCore
            AZ::AzFramework
        PUBLIC
            Gem::Atom_RHI.Public
)

################################################################################
# Private
################################################################################
ly_add_target(
    NAME Atom_RPI.Private ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Atom_RPI.Private.a218db9eb2114477b46600fea4441a6c.v0.1.0
    FILES_CMAKE
        atom_rpi_private_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AtomCore
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_RPI.Public
            Gem::Atom_RHI.Public
            Gem::Atom_RHI.Reflect
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Atom_RPI.Edit STATIC
        NAMESPACE Gem
        FILES_CMAKE
            atom_rpi_edit_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AtomCore
                AZ::AzToolsFramework
                Gem::Atom_RPI.Public
    )

    ly_add_target(
        NAME Atom_RPI.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            atom_rpi_private_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PUBLIC
                RPI_EDITOR # Compile RPI Editor module
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AtomCore
                AZ::AzCore
                AZ::AzFramework
                Gem::Atom_RPI.Public
                Gem::Atom_RHI.Public
    )

    ly_add_target(
        NAME Atom_RPI.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Atom_RPI.Editor.a218db9eb2114477b46600fea4441a6c.v0.1.0
        FILES_CMAKE
            atom_rpi_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AtomCore
                AZ::AzCore
                AZ::AzToolsFramework
                Gem::Atom_RPI.Editor.Static
                Gem::Atom_RPI.Edit
                Gem::Atom_RPI.Public
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Atom_RPI.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atom_rpi_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AtomCore
                AZ::AzTest
                AZ::AzFramework
                AZ::AzToolsFramework
                Legacy::CryCommon
                Gem::Atom_RPI.Public
                Gem::Atom_RHI.Public
                Gem::Atom_RPI.Edit
    )
    ly_add_googletest(
        NAME Gem::Atom_RPI.Tests
    )

endif()


################################################################################
# Builders
################################################################################

if(PAL_TRAIT_BUILD_HOST_TOOLS)


    ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})
    ly_get_list_relative_pal_filename(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

    include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) #for PAL_TRAIT_BUILD_ATOM_RPI_ASSETS_SUPPORTED

    if(NOT PAL_TRAIT_BUILD_ATOM_RPI_ASSETS_SUPPORTED)

        # Create a stub
        ly_add_target(
            NAME Atom_RPI.Builders MODULE
            NAMESPACE Gem
            OUTPUT_NAME Gem.Atom_RPI.Builders.a218db9eb2114477b46600fea4441a6c.v0.1.0
            FILES_CMAKE
                atom_rpi_builders_stub_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                PUBLIC
                    Include
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzCore
        )

        return()

    endif()
    
    ly_add_target(
        NAME Atom_RPI.Builders.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            atom_rpi_builders_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Source/RPI.Builders
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AtomCore
                AZ::AzCore
                AZ::AzFramework
                AZ::SceneCore
                AZ::SceneData
                AZ::AssetBuilderSDK
                Gem::Atom_RPI.Edit
                Gem::Atom_RPI.Public
                Gem::Atom_RHI.Public
    )

    ly_add_target(
        NAME Atom_RPI.Builders MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Atom_RPI.Builders.a218db9eb2114477b46600fea4441a6c.v0.1.0
        FILES_CMAKE
            atom_rpi_builders_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Source/RPI.Builders
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AtomCore
                AZ::AzCore
                AZ::AzFramework
                AZ::SceneCore
                AZ::AssetBuilderSDK
                Gem::Atom_RHI.Public
                Gem::Atom_RPI.Edit
                Gem::Atom_RPI.Public
                Gem::Atom_RPI.Builders.Static
    )

    if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

        ly_add_target(
            NAME Atom_RPI.Builders.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                atom_rpi_builders_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    Tests
                    Source/RPI.Builders
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AtomCore
                    AZ::AzTest
                    AZ::AzFramework
                    AZ::AzToolsFramework
                    AZ::AssetBuilderSDK
                    AZ::SceneCore
                    Legacy::CryCommon
                    Gem::Atom_RPI.Public
                    Gem::Atom_RHI.Public
                    Gem::Atom_RPI.Edit
                    Gem::Atom_RPI.Builders.Static
        )
        ly_add_googletest(
            NAME Gem::Atom_RPI.Builders.Tests
        )
    endif()
endif()

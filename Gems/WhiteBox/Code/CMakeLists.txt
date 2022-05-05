#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")
set(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_WHITEBOX_SUPPORTED)
    ly_add_target(
        NAME WhiteBox ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            whitebox_unsupported_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME WhiteBox.Editor GEM_MODULE
            NAMESPACE Gem
            FILES_CMAKE
                whitebox_unsupported_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                PUBLIC
                    Include
            COMPILE_DEFINITIONS
                PRIVATE
                    WHITE_BOX_EDITOR
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzCore
            RUNTIME_DEPENDENCIES
                Gem::EditorPythonBindings.Editor
        )
    endif()
    return()
endif()

ly_add_target(
    NAME WhiteBox.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        whitebox_supported_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Gem::Atom_RPI.Public
            Gem::Atom_Feature_Common.Static
            Gem::AtomLyIntegration_CommonFeatures.Static
            Legacy::CryCommon
)

ly_add_target(
    NAME WhiteBox ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        whitebox_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::WhiteBox.Static
)

# use the above WhiteBox module in runtimes:
ly_create_alias(NAME WhiteBox.Clients  NAMESPACE Gem TARGETS Gem::WhiteBox)
ly_create_alias(NAME WhiteBox.Servers  NAMESPACE Gem TARGETS Gem::WhiteBox)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME WhiteBox.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            whitebox_editor_supported_files.cmake
        PLATFORM_INCLUDE_FILES
            ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_tools.cmake
            ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/whitebox_editor_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                _USE_MATH_DEFINES
                OM_STATIC_BUILD
            PUBLIC
                WHITE_BOX_EDITOR
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Legacy::CryCommon
                Legacy::EditorCommon
                Legacy::Editor.Headers
                Gem::WhiteBox.Static
    )

    ly_add_target(
        NAME WhiteBox.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            whitebox_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::WhiteBox.Editor.Static
    )

    # use the above WhiteBox.Editor module in dev tools:
    ly_create_alias(NAME WhiteBox.Tools    NAMESPACE Gem TARGETS Gem::WhiteBox.Editor)
    ly_create_alias(NAME WhiteBox.Builders NAMESPACE Gem TARGETS Gem::WhiteBox.Editor)


endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME WhiteBox.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            whitebox_tests_supported_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTestShared
                AZ::AzTest
                Gem::WhiteBox.Static
    )

    ly_add_googletest(
        NAME Gem::WhiteBox.Tests
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME WhiteBox.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                whitebox_editor_tests_supported_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTestShared
                    AZ::AzManipulatorTestFramework.Static
                    Gem::WhiteBox.Editor.Static
        )

        ly_add_googletest(
            NAME Gem::WhiteBox.Editor.Tests
        )

        ly_add_target(
            NAME WhiteBox.Editor.Physics.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                whitebox_editor_physics_tests_supported_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTestShared
                    AZ::AzManipulatorTestFramework.Static
                    Gem::WhiteBox.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::PhysX.Editor
        )

        ly_add_googletest(
            NAME Gem::WhiteBox.Editor.Physics.Tests
        )

    endif()
endif()

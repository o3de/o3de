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
        NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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
            NAME ${gem_name}.Editor GEM_MODULE
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

    # Inject the gem name into the Unsupported Module source file
    ly_add_source_properties(
        SOURCES
            Source/WhiteBoxModuleUnsupported.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})
    return()
endif()

ly_add_target(
    NAME ${gem_name}.Static STATIC
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
            Gem::Atom_Feature_Common.Public
            Gem::CommonFeaturesAtom.Static
            Legacy::CryCommon
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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
            Gem::${gem_name}.Static
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/WhiteBoxModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# use the above ${gem_name} module in runtimes:
ly_create_alias(NAME ${gem_name}.Clients  NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers  NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified  NAMESPACE Gem TARGETS Gem::${gem_name})

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
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
                Gem::${gem_name}.Static
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE

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
                Gem::${gem_name}.Editor.Static
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/${gem_name}EditorModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    # use the above ${gem_name}.Editor module in dev tools:
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)


endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                Gem::${gem_name}.Static
    )

    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        LABELS REQUIRES_tiaf
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                    Gem::${gem_name}.Editor.Static
        )

        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
        )

        ly_add_target(
            NAME ${gem_name}.Editor.Physics.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                    Gem::${gem_name}.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::PhysX.Editor
        )

        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Physics.Tests
            LABELS REQUIRES_tiaf
        )

    endif()
endif()

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

if(NOT PAL_TRAIT_BUILD_P4PLUGIN_SUPPORTED)
    return()
endif()


if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME ${gem_name}.Editor.Static GEM_MODULE
        NAMESPACE Gem        
        AUTOMOC
        AUTOUIC
        FILES_CMAKE
            perforce_editor_supported_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Core
                3rdParty::Qt::Widgets
                AZ::AzCore
                Legacy::CryCommon
                Legacy::EditorLib
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            perforce_editor_shared_files.cmake
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
    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                perforce_editor_tests_supported_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTestShared
                    Gem::${gem_name}.Editor.Static
        )

        ly_add_googletest(
            NAME Gem::${gem_name}.Editor.Tests
        )
    endif()
endif()


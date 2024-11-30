#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    set(additional_tool_deps AZ::AzToolsFramework)
endif()


ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        awscore_files.cmake
         ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            ${pal_dir}
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AWSNativeSDKInit
        PUBLIC
            3rdParty::AWSNativeSDK::AWSCore
)

ly_add_target( 
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awscore_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
        PUBLIC
            Gem::${gem_name}.Static
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/AWSCoreModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# clients and servers will use the above Gem::${gem_name} module.
ly_create_alias(
    NAME ${gem_name}.Servers
    NAMESPACE Gem
    TARGETS
        Gem::${gem_name}
)

ly_create_alias(
    NAME ${gem_name}.Clients
    NAMESPACE Gem
    TARGETS
        Gem::${gem_name}
)

ly_create_alias(
    NAME ${gem_name}.Unified
    NAMESPACE Gem
    TARGETS
        Gem::${gem_name}
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)

    include(${pal_dir}/PAL_traits_editor_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            awscore_editor_files.cmake
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzQtComponents
                3rdParty::Qt::Core
                3rdParty::Qt::Widgets
                Gem::${gem_name}.Static
            PUBLIC
                ${additional_tool_deps}
                3rdParty::AWSNativeSDK::AWSCore
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            awscore_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                Gem::${gem_name}.Editor.Static
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/AWSCoreEditorModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    if (PAL_TRAIT_ENABLE_RESOURCE_MAPPING_TOOL)

        # This target is not a real gem module
        # It is not meant to be loaded by the ModuleManager in C++
        ly_add_target(
            NAME ${gem_name}.ResourceMappingTool MODULE
            NAMESPACE Gem
            OUTPUT_SUBDIRECTORY AWSCoreEditorQtBin
            FILES_CMAKE
                awscore_resourcemappingtool_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    Gem::${gem_name}.Editor.Static
            RUNTIME_DEPENDENCIES
                3rdParty::pyside2

        )
        ly_add_dependencies(${gem_name}.Editor ${gem_name}.ResourceMappingTool)

        ly_install_directory(DIRECTORIES Tools/ResourceMappingTool)
        
    endif()

    # Builders and Tools (such as the Editor use ${gem_name}.Editor) use the .Editor module above.
    ly_create_alias(
        NAME ${gem_name}.Tools
        NAMESPACE Gem
        TARGETS
            Gem::${gem_name}
            Gem::${gem_name}.Editor
    )

    ly_create_alias(
        NAME ${gem_name}.Builders
        NAMESPACE Gem
        TARGETS
            Gem::${gem_name}
            Gem::${gem_name}.Editor
    )

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME ${gem_name}.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            awscore_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Include
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                AZ::AWSNativeSDKTestLibs
                Gem::${gem_name}.Static
    )

    ly_add_googletest(
        NAME Gem::${gem_name}.Tests
        LABELS REQUIRES_tiaf
    )

    ly_add_target_files(
        TARGETS
            ${gem_name}.Tests
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/../Assets/resource_mapping_schema.json
        OUTPUT_SUBDIRECTORY
            Gems/${gem_name}
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                awscore_editor_tests_files.cmake
                ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_editor_files.cmake
                ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                    ${pal_dir}
                    Include
                    Tests
            COMPILE_DEFINITIONS
                PRIVATE
                    AWSCORE_EDITOR
            BUILD_DEPENDENCIES
                PRIVATE
                    3rdParty::Qt::Gui
                    3rdParty::Qt::Widgets
                    AZ::AzTest
                    AZ::AWSNativeSDKTestLibs
                    Gem::${gem_name}.Static
                    Gem::${gem_name}.Editor.Static
        )
        set(SUPPORTED_PLATFORMS "Windows")
        if ("${PAL_PLATFORM_NAME}" IN_LIST SUPPORTED_PLATFORMS)
            ly_add_googletest(
                NAME Gem::${gem_name}.Editor.Tests
                LABELS REQUIRES_tiaf
            )
        endif()
    endif()
endif()

ly_add_target_files(
    TARGETS
        ${gem_name}
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../Assets/resource_mapping_schema.json
    OUTPUT_SUBDIRECTORY
        Gems/${gem_name}
)

ly_install_directory(DIRECTORIES Tools/ResourceMappingTool)

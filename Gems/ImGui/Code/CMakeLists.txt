#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(config_base_defines $<IF:$<CONFIG:release>,IMGUI_DISABLED,IMGUI_ENABLED>)

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# This library is the 3rdParty imgui that is in the Gem's External
ly_add_target(
    NAME ImGui.imguilib ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        imgui_lib_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/imgui_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        INTERFACE
            ../External/ImGui/v1.82
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PUBLIC
            ${config_base_defines}
        INTERFACE
            $<$<NOT:$<BOOL:${LY_MONOLITHIC_GAME}>>:IMGUI_API_IMPORT>
            IMGUI_INCLUDE_IMGUI_USER_H
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)

ly_add_target(
    NAME ImGui.ImGuiLYUtils STATIC
    NAMESPACE Gem
    FILES_CMAKE
        imgui_lyutils_static_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ImGui.imguilib
            Legacy::CryCommon
    RUNTIME_DEPENDENCIES
        Gem::ImGui.imguilib
)

ly_add_target(
    NAME ImGui.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        imgui_common_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ImGui.ImGuiLYUtils
        PRIVATE
            Gem::LmbrCentral
)

ly_add_target(
    NAME ImGui ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        imgui_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC 
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::ImGui.Static
    RUNTIME_DEPENDENCIES
        Gem::LmbrCentral
)

# Load the above "Gem::ImGui" module in Clients and Servers:
ly_create_alias(NAME ImGui.Clients NAMESPACE Gem TARGETS Gem::ImGui)
ly_create_alias(NAME ImGui.Servers NAMESPACE Gem TARGETS Gem::ImGui)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ImGui.Editor GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            imgui_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                IMGUI_GEM_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Editor
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::ImGui.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral.Editor
    )

    # Load the above "Gem::ImGui.Editor" module in only tools and builders.
    ly_create_alias(NAME ImGui.Builders NAMESPACE Gem TARGETS Gem::ImGui.Editor)
    ly_create_alias(NAME ImGui.Tools    NAMESPACE Gem TARGETS Gem::ImGui.Editor)

endif()

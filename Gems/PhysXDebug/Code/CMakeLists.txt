#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(physx_pal_source_dir ${LY_ROOT_FOLDER}/Gems/PhysX/Code/Source/Platform/${PAL_PLATFORM_NAME})

include(${physx_pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # for PAL_TRAIT_PHYSX_SUPPORTED

if(PAL_TRAIT_PHYSX_SUPPORTED)
    set(physx_dependency 3rdParty::PhysX)
    set(physx_files physxdebug_files.cmake)
    set(physx_editor_files physxdebug_editor_files.cmake)
else()
    set(physx_files physxdebug_unsupported_files.cmake)
    set(physx_editor_files physxdebug_unsupported_files.cmake)
endif()

ly_add_target(
    NAME PhysXDebug ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME PhysXDebug.Gem
    FILES_CMAKE
        ${physx_files}
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            ${physx_dependency}
            Legacy::CryCommon
            Gem::PhysX
            Gem::ImGui.imguilib
            Gem::ImGui
    RUNTIME_DEPENDENCIES
        Gem::PhysX
        Gem::ImGui
)
# use the PhysXDebug module in Clients and Servers:
ly_create_alias(NAME PhysXDebug.Clients NAMESPACE Gem TARGETS Gem::PhysXDebug)
ly_create_alias(NAME PhysXDebug.Servers NAMESPACE Gem TARGETS Gem::PhysXDebug)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME PhysXDebug.Editor GEM_MODULE
        NAMESPACE Gem
        OUTPUT_NAME PhysXDebug.Editor.Gem
        FILES_CMAKE
            ${physx_editor_files}
        COMPILE_DEFINITIONS
            PRIVATE
                PHYSXDEBUG_GEM_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                ${physx_dependency}
                Legacy::CryCommon
                Legacy::Editor.Headers
                AZ::AzToolsFramework
                Gem::PhysX.Editor
                Gem::ImGui.imguilib
                Gem::ImGui.Editor
        RUNTIME_DEPENDENCIES
            Gem::PhysX.Editor
            Gem::ImGui.Editor
    )
    # use the PhysXDebug.Editor module in dev tools:
    ly_create_alias(NAME PhysXDebug.Builders NAMESPACE Gem TARGETS Gem::PhysXDebug.Editor)
    ly_create_alias(NAME PhysXDebug.Tools    NAMESPACE Gem TARGETS Gem::PhysXDebug.Editor)

endif()

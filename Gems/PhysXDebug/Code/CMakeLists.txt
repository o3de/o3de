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

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME PhysXDebug.Editor GEM_MODULE

        NAMESPACE Gem
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
                Gem::PhysX
                Gem::ImGui.imguilib
                Gem::ImGui
        RUNTIME_DEPENDENCIES
            Gem::PhysX.Editor
            Gem::ImGui.Editor
    )
endif()

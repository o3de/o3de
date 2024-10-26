#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atombridge_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PRIVATE
            ENABLE_ATOM_DEBUG_DISPLAY=1
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AtomCore
            AZ::AzFramework
            Gem::Atom_RPI.Public
            Gem::Atom_Bootstrap.Headers
            Legacy::CryCommon
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atombridge_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/additional_${PAL_PLATFORM_NAME_LOWERCASE}_runtime_deps.cmake
    COMPILE_DEFINITIONS
        PRIVATE
            ENABLE_ATOM_DEBUG_DISPLAY=1
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Static
    RUNTIME_DEPENDENCIES
        Gem::Atom_RHI.Private
        Gem::Atom_RPI.Private
        Gem::Atom_RHI_Null.Private
        Gem::Atom_Feature_Common
        Gem::Atom_Bootstrap
        Gem::Atom_Component_DebugCamera
        Gem::AtomImGuiTools
        Gem::CommonFeaturesAtom
        Gem::EMotionFX_Atom
        Gem::ImguiAtom
        Gem::AtomFont
        Gem::AtomViewportDisplayInfo
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/AtomBridgeModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

ly_add_target(
    NAME ${gem_name}.Headless ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atombridge_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PRIVATE
            ENABLE_ATOM_DEBUG_DISPLAY=1
            O3DE_HEADLESS_SERVER=1
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Static
    RUNTIME_DEPENDENCIES
        Gem::Atom_RHI.Private
        Gem::Atom_RPI.Private
        Gem::Atom_RHI_Null.Private
        Gem::Atom_Feature_Common.Headless
        Gem::Atom_Bootstrap
        Gem::Atom_Component_DebugCamera
        Gem::AtomImGuiTools
        Gem::CommonFeaturesAtom
        Gem::EMotionFX_Atom
        Gem::ImguiAtom
        Gem::AtomFont
        Gem::AtomViewportDisplayInfo
)

# Any 'runtime-like' applications should use Gem::${gem_name}:
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.HeadlessServers NAMESPACE Gem TARGETS Gem::${gem_name}.Headless)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    set(additional_tool_deps ${pal_dir}/additional_${PAL_PLATFORM_NAME_LOWERCASE}_tool_deps.cmake)
    foreach(pal_tools_platform ${LY_PAL_TOOLS_ENABLED})
        string(TOLOWER ${pal_tools_platform} pal_tools_platform_lowercase)
        ly_get_list_relative_pal_filename(pal_runtime_dependencies_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${pal_tools_platform})
        list(APPEND additional_tool_deps ${pal_runtime_dependencies_source_dir}/additional_${pal_tools_platform_lowercase}_tool_deps.cmake)
    endforeach()

    ly_add_target(
        NAME ${gem_name}.Editor ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atombridge_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        PLATFORM_INCLUDE_FILES
           ${additional_tool_deps}
        COMPILE_DEFINITIONS
            PRIVATE
                EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AssetBuilderSDK
                Gem::Atom_Utils.Static
                Gem::${gem_name}.Static
        RUNTIME_DEPENDENCIES
            Gem::Atom_RHI.Private
            Gem::Atom_RPI.Builders
            Gem::Atom_RPI.Editor
            Gem::Atom_RHI_Null.Private
            Gem::Atom_RHI_Null.Builders
            Gem::Atom_Feature_Common.Builders
            Gem::Atom_Feature_Common.Editor
            Gem::Atom_Bootstrap
            Gem::AtomShader.Builders
            Gem::Atom_Component_DebugCamera
            Gem::AtomImGuiTools
            Gem::AtomRenderOptions.Editor
            Gem::CommonFeaturesAtom.Editor
            Gem::EMotionFX_Atom.Editor
            Gem::ImageProcessingAtom.Editor
            Gem::ImguiAtom
            Gem::AtomFont
            Gem::AtomToolsFramework.Editor
            Gem::AtomViewportDisplayInfo
            Gem::AtomViewportDisplayIcons.Editor
            Gem::MaterialCanvas.Builders
            Gem::MaterialEditor.Builders
            Gem::PassCanvas.Builders
            Gem::ShaderManagementConsole.Builders
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/AtomBridgeEditorModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})


    # Any 'tool' and 'builder' type applications should use Gem::${gem_name}.Editor:
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
endif()

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This is an editor/tools-only Gem for upgrading old LyShine v1/v2 .uicanvas assets
# to Shine v3 format. It depends on the engine's SliceComponent (still in AzCore)
# to resolve SliceReferences and DataPatches in old canvases.

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${gem_name}.Tools.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            lyshinetoshine_tools_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${CMAKE_CURRENT_LIST_DIR}/../../Shine/Code/Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
            PUBLIC
                AZ::AssetBuilderSDK
                Gem::Shine.Static
    )

    ly_add_target(
        NAME ${gem_name}.Tools GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            lyshinetoshine_module_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::${gem_name}.Tools.Static
        RUNTIME_DEPENDENCIES
            Gem::Shine.Tools
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/LYShineToShineModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    ly_create_alias(NAME ${gem_name}.Tools.API NAMESPACE Gem TARGETS Gem::${gem_name}.Tools)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Tools)
endif()

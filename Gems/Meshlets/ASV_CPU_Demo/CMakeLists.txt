#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME AtomSampleViewer.Lib.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atomsampleviewergem_lib_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Lib
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzGameFramework
            Gem::Atom_AtomBridge.Static
            Gem::Atom_RPI.Public
            Gem::Atom_Utils.Static
)

ly_add_target(
    NAME AtomSampleViewer.Private.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atomsampleviewergem_private_files.cmake
        ${pal_dir}/atomsampleviewer_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Lib
            Source
            ${pal_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzGameFramework
            Gem::Atom_AtomBridge.Static
            Gem::Atom_Feature_Common.Static
            Gem::Atom_Component_DebugCamera.Static
            Gem::AtomSampleViewer.Lib.Static
            Gem::Meshlets.Static
            Gem::Profiler.Static
)

ly_add_target(
    NAME AtomSampleViewer ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atomsampleviewergem_private_shared_files.cmake
        ../../atomsampleviewer_asset_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/additional_${PAL_PLATFORM_NAME_LOWERCASE}_runtime_library.cmake
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzGameFramework
            Gem::Atom_AtomBridge.Static
            Gem::AtomSampleViewer.Private.Static
            Gem::ImGui.imguilib
            Gem::Meshlets.Static
)

# if enabled, AtomSampleViewer is used by the Client and ServerLauncher
ly_create_alias(NAME AtomSampleViewer.Clients  NAMESPACE Gem TARGETS Gem::AtomSampleViewer)
ly_create_alias(NAME AtomSampleViewer.Servers  NAMESPACE Gem TARGETS Gem::AtomSampleViewer)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME AtomSampleViewer.Tools.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            atomsampleviewergem_tools_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                .
                Tools
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzGameFramework
                Gem::Atom_AtomBridge.Static
                Gem::AtomSampleViewer.Lib.Static
            PUBLIC
                Gem::Atom_RPI.Edit
    )

    ly_add_target(
        NAME AtomSampleViewer.Tools GEM_MODULE

        NAMESPACE Gem
        FILES_CMAKE
            atomsampleviewergem_tools_shared_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Source/Platform/${PAL_PLATFORM_NAME}
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzCore
                Gem::AtomSampleViewer.Tools.Static
        RUNTIME_DEPENDENCIES
            Gem::AtomSampleViewer
    )

    # The AtomSampleViewer.Tools target is the real GEM_MODULE target made above, but the AssetBuilder/AssetProcessor
    # also needs that target, so alias the "Builders" variant to it
    ly_create_alias(NAME AtomSampleViewer.Builders NAMESPACE Gem TARGETS Gem::AtomSampleViewer.Tools)
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_SUPPORTS_TESTS)
    ly_add_target(
        NAME AtomSampleViewer.Tests MODULE

        NAMESPACE Gem
        FILES_CMAKE
            atomsampleviewer_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::AtomSampleViewer
    )
    ly_add_googletest(
        NAME Gem::AtomSampleViewer.Tests
        TARGET Gem::AtomSampleViewer.Tests
    )
endif()


################################################################################
# Gem dependencies
################################################################################

ly_enable_gems(PROJECT_NAME AtomSampleViewer GEM_FILE enabled_gems.cmake)

# If we build a server, then apply the gems to the server
if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    set_property(GLOBAL APPEND PROPERTY LY_LAUNCHER_SERVER_PROJECTS AtomSampleViewer)
endif()

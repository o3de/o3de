#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/OpenParticleSystem/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/OpenParticleSystem/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${o3de_gem_restricted_path}" ${o3de_gem_path} ${o3de_gem_parent_relative_path})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the OpenParticleSystem.Static target
# Note: We include the common files and the platform specific files which are set in openparticlesystem_common_files.cmake
# and in ${pal_dir}/openparticlesystem_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME SimuCore.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        openparticlesystem_simucore_core.cmake
    NO_UNITY
    INCLUDE_DIRECTORIES
        PUBLIC
            SimuCore/core/include
        PRIVATE
            SimuCore/core/src
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME SimuCoreParticleModules.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        openparticlesystem_simucore_modules.cmake
    NO_UNITY
    INCLUDE_DIRECTORIES
        PUBLIC
            SimuCore/modules/particle/core/include
        PRIVATE
            SimuCore/modules/particle/core/src
    BUILD_DEPENDENCIES
        PUBLIC
            #AZ::AzCore
            #AZ::AzFramework
            Gem::SimuCore.Static
)

ly_add_target(
    NAME OpenParticleSystem.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        openparticlesystem_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            SimuCore/core/include
            SimuCore/modules/particle/core/include
        PRIVATE
            Source/Runtime
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
        PUBLIC
            Gem::Atom_RPI.Public
            Gem::Atom_Utils.Static
            Gem::Atom_Feature_Common.Public
            Gem::SimuCore.Static
            Gem::SimuCoreParticleModules.Static
            Gem::EMotionFX.Static
            Gem::EMotionFX_Atom.Static
)

# Here add OpenParticleSystem target, it depends on the OpenParticleSystem.Static
ly_add_target(
    NAME OpenParticleSystem ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME OpenParticleSystem.Gem
    FILES_CMAKE
        openparticlesystem_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source/Runtime
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::OpenParticleSystem.Static
            AZ::AzCore
            AZ::AzFramework
)

# By default, we will specify that the above target OpenParticleSystem would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME OpenParticleSystem.Clients NAMESPACE Gem TARGETS Gem::OpenParticleSystem)
ly_create_alias(NAME OpenParticleSystem.Servers NAMESPACE Gem TARGETS Gem::OpenParticleSystem)

# If we are on a host platform, we want to add the host tools targets like the OpenParticleSystem.Editor target which
# will also depend on OpenParticleSystem.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME OpenParticleSystem.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            openparticlesystem_editor_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                Include
                Source
            PRIVATE
                Source/Runtime
                Source/Thumbnail
        COMPILE_DEFINITIONS
            PRIVATE
                PARTICLE_EDITOR
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzCore
                AZ::AzToolsFramework
                Legacy::EditorCore
                Gem::OpenParticleSystem.Static
            PRIVATE
                Gem::Atom_RPI.Edit
                Gem::AtomToolsFramework.Static
    )

    ly_add_target(
        NAME OpenParticleSystem.Editor GEM_MODULE
        NAMESPACE Gem
        AUTORCC
        OUTPUT_NAME OpenParticleSystem.Editor.Gem
        FILES_CMAKE
            openparticlesystem_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
                Source/Runtime
                Source/Thumbnail
        COMPILE_DEFINITIONS
            PRIVATE
                PARTICLE_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::OpenParticleSystem.Editor.Static
                AZ::AzCore
                AZ::AzToolsFramework
                Gem::AtomToolsFramework.Static
                Gem::OpenParticleSystemEditor
    )

    ly_add_target(
        NAME OpenParticleSystem.Builder.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            openparticlesystem_builder_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Builder
                Source/Runtime
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::SceneCore
                AZ::AssetBuilderSDK
                Gem::Atom_RPI.Edit
                Gem::Atom_RPI.Public
                Gem::Atom_RHI.Public
            PUBLIC
                Gem::OpenParticleSystem.Editor.Static
                Gem::OpenParticleSystem.Static

    )

    ly_add_target(
        NAME OpenParticleSystem.Builder GEM_MODULE
        NAMESPACE Gem
        OUTPUT_NAME OpenParticleSystem.Builder.Gem
        FILES_CMAKE
            openparticlesystem_builder_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Builder
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::OpenParticleSystem.Builder.Static
            PRIVATE
                AZ::AzCore
                AZ::SceneCore
                AZ::AssetBuilderSDK
    )

    #ly_enable_gems(GEMS OpenParticleSystem)

endif()

# By default, we will specify that the above target OpenParticleSystem would be used by
# Tool and Builder type targets when this gem is enabled.  If you don't want it
# active in Tools or Builders by default, delete one of both of the following lines:
ly_create_alias(NAME OpenParticleSystem.Builders NAMESPACE Gem TARGETS Gem::OpenParticleSystem.Builder)
ly_create_alias(NAME OpenParticleSystem.Tools    NAMESPACE Gem TARGETS Gem::OpenParticleSystem.Editor)

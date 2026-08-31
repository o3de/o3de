#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/Meshlets/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/Meshlets/Code
#o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_name}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
#include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Headers-only target so other gems (e.g. AtomLyIntegration::CommonFeatures, for the
# MeshComponent "Use Virtual Geometry (Meshlets)" toggle) can consume the Meshlets
# feature processor's public interface without linking to the implementation.
ly_add_target(
    NAME ${gem_name}.Public HEADERONLY
    NAMESPACE Gem
    FILES_CMAKE
        meshlets_public_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
            Gem::Atom_RPI.Public
)

# Reflect-tier target: shared by runtime and builder. Defines the on-disk
# .azmeshletpack format (header, section ToC, section-kind enum) and the
# MeshletPackAsset class. Has no Atom_RPI dep so the builder can link it
# without pulling the renderer.
#
# STATIC linkage matches the rest of the Meshlets gem's static libs
# (Meshlets.Static, Meshlets.Builders.Static, Meshlets.Editor.Static).
# Atom_RHI.Reflect uses PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE because
# it has the supporting __declspec(dllexport) decorations + an
# ATOM_RHI_REFLECT_EXPORTS macro; we don't decorate Meshlets headers,
# so STATIC is the right type -- otherwise downstream consumers hit
# unresolved-external link errors on Windows non-monolithic builds.
ly_add_target(
    NAME ${gem_name}.Reflect STATIC
    NAMESPACE Gem
    FILES_CMAKE
        meshlets_reflect_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
)

# Add the Meshlets.Static target
# Note: We include the common files and the platform specific files which are set in meshlets_common_files.cmake
# and in ${pal_dir}/meshlets_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        meshlets_files.cmake
#        ${pal_dir}/meshlets_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            Source
            Source/${gem_name}
        PRIVATE
            Source
            Source/${gem_name}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            # Removed Gem::CommonFeaturesAtom.Static: no source file in this gem ever
            # actually included a CommonFeatures header. Keeping it would create a
            # circular dependency now that CommonFeatures consumes
            # MeshletsFeatureProcessorInterface for its "Use Meshlets" toggle.
            Gem::${gem_name}.Public
            Gem::${gem_name}.Reflect
            Gem::Atom_RPI.Public
            Gem::Atom_Feature_Common.Public
)

# Here add ${gem_name} target, it depends on the ${gem_name}.Static
ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        meshlets_shared_files.cmake
#        ${pal_dir}/meshlets_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            Source
            Source/${gem_name}
        PRIVATE
            Source
            Source/${gem_name}
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Static
)

# Inject the gem name into the Module source file
ly_add_source_properties(
    SOURCES
        Source/MeshletsModule.cpp
    PROPERTY COMPILE_DEFINITIONS
        VALUES
            O3DE_GEM_NAME=${gem_name}
            O3DE_GEM_VERSION=${gem_version})

# By default, we will specify that the above target ${gem_name} would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

# If we are on a host platform, we want to add the host tools targets like the ${gem_name}.Editor target which
# will also depend on ${gem_name}.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # Builder-tier static lib: the meshletization pipeline + rule reflect classes.
    # Host-tools only (builders run inside AssetProcessor, not at runtime).
    ly_add_target(
        NAME ${gem_name}.Builders.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            meshlets_builders_files.cmake
        INCLUDE_DIRECTORIES
            PUBLIC
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzCore
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                AZ::SceneCore
                AZ::SceneData
                Gem::${gem_name}.Reflect
                Gem::Atom_RHI.Reflect
                Gem::Atom_RPI.Public
                3rdParty::meshoptimizer
    )

    ly_add_target(
        NAME ${gem_name}.Builders GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            meshlets_builders_module_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::${gem_name}.Builders.Static
    )

    # Inject the gem name into the Builders module source so AZ_DECLARE_MODULE_CLASS
    # expands to the gem-prefixed CreateModuleClass_Gem_Meshlets_Builders export
    # symbol AP looks up. Without this the DLL exports the generic CreateModuleClass
    # and AP silently fails to load the module.
    ly_add_source_properties(
        SOURCES
            Source/Builders/MeshletsBuildersModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    ly_add_target(
        NAME ${gem_name}.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            meshlets_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                External/Include
                Source
                Source/${gem_name}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::${gem_name}.Static
            PUBLIC
                # PUBLIC so IMGUI_ENABLED and imgui headers propagate to the GEM_MODULE
                # that links this static lib -- the editor component's class definition
                # is guarded on IMGUI_ENABLED and must be identical in every TU.
                Gem::ImGui.imguilib
                Gem::ImGui.Static
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        FILES_CMAKE
            meshlets_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Source/${gem_name}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::${gem_name}.Editor.Static
    )

    # Inject the gem name into the Module source file
    ly_add_source_properties(
        SOURCES
            Source/MeshletsEditorModule.cpp
        PROPERTY COMPILE_DEFINITIONS
            VALUES
                O3DE_GEM_NAME=${gem_name}
                O3DE_GEM_VERSION=${gem_version})

    # The .Tools alias loads the Editor module in tool contexts (e.g. O3DE Editor).
    # The .Builders alias is NOT created here because a real Meshlets.Builders GEM_MODULE
    # target already exists above -- creating an alias with the same name would conflict.
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ${gem_name}.Static
    if(PAL_TRAIT_MESHLETS_TEST_SUPPORTED)
        # We support ${gem_name}.Tests on this platform, add ${gem_name}.Tests target which depends on ${gem_name}.Static
        ly_add_target(
            NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                meshlets_files.cmake
                meshlets_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::${gem_name}.Static
        )

        # Add ${gem_name}.Tests to googletest
        ly_add_googletest(
            NAME Gem::${gem_name}.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_MESHLETS_EDITOR_TEST_SUPPORTED)
            # We support ${gem_name}.Editor.Tests on this platform, add ${gem_name}.Editor.Tests target which depends on ${gem_name}.Editor
            ly_add_target(
                NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    meshlets_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::${gem_name}.Editor
            )

            # Add ${gem_name}.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::${gem_name}.Editor.Tests
            )
        endif()

        ly_add_target(
            NAME ${gem_name}.Builders.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                meshlets_builders_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::${gem_name}.Builders.Static
        )

        ly_add_googletest(
            NAME Gem::${gem_name}.Builders.Tests
        )
    endif()
endif()

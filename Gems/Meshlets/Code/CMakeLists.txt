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

# Add the Meshlets.Static target
# Note: We include the common files and the platform specific files which are set in meshlets_common_files.cmake
# and in ${pal_dir}/meshlets_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME Meshlets.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        meshlets_files.cmake
#        ${pal_dir}/meshlets_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            External/Include
            Source
            Source/Meshlets
        PRIVATE
            External/Include
            Source
            Source/Meshlets
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::AtomLyIntegration_CommonFeatures.Static
            Gem::Atom_RPI.Public
)

# Here add Meshlets target, it depends on the Meshlets.Static
ly_add_target(
    NAME Meshlets ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        meshlets_shared_files.cmake
#        ${pal_dir}/meshlets_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            Source
            Source/Meshlets
        PRIVATE
            Source
            Source/Meshlets
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Meshlets.Static
)

# By default, we will specify that the above target Meshlets would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME Meshlets.Clients NAMESPACE Gem TARGETS Gem::Meshlets)
ly_create_alias(NAME Meshlets.Servers NAMESPACE Gem TARGETS Gem::Meshlets)
ly_create_alias(NAME Meshlets.Unified NAMESPACE Gem TARGETS Gem::Meshlets)

# If we are on a host platform, we want to add the host tools targets like the Meshlets.Editor target which
# will also depend on Meshlets.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Meshlets.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            meshlets_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                External/Include
                Source
                Source/Meshlets
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::Meshlets.Static
    )

    ly_add_target(
        NAME Meshlets.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            meshlets_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Source/Meshlets
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::Meshlets.Editor.Static
    )

    # By default, we will specify that the above target Meshlets would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME Meshlets.Tools    NAMESPACE Gem TARGETS Gem::Meshlets.Editor)
    ly_create_alias(NAME Meshlets.Builders NAMESPACE Gem TARGETS Gem::Meshlets.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for Meshlets.Static
    if(PAL_TRAIT_MESHLETS_TEST_SUPPORTED)
        # We support Meshlets.Tests on this platform, add Meshlets.Tests target which depends on Meshlets.Static
        ly_add_target(
            NAME Meshlets.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                    Gem::Meshlets.Static
        )

        # Add Meshlets.Tests to googletest
        ly_add_googletest(
            NAME Gem::Meshlets.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_MESHLETS_EDITOR_TEST_SUPPORTED)
            # We support Meshlets.Editor.Tests on this platform, add Meshlets.Editor.Tests target which depends on Meshlets.Editor
            ly_add_target(
                NAME Meshlets.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                        Gem::Meshlets.Editor
            )

            # Add Meshlets.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::Meshlets.Editor.Tests
            )
        endif()
    endif()
endif()

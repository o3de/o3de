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
#       i.e. It could here in our gem : Gems/MiniAudio/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/MiniAudio/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# The MiniAudio.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME MiniAudio.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        miniaudio_api_files.cmake
        ${pal_dir}/miniaudio_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
)

# The MiniAudio.Private.Object target is an internal target
# It should not be used outside of this Gems CMakeLists.txt
ly_add_target(
    NAME MiniAudio.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        miniaudio_private_files.cmake
        ${pal_dir}/miniaudio_private_files.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
        PRIVATE
            3rdParty::miniaudio
            3rdParty::stb_vorbis
)

# Here add MiniAudio target, it depends on the Private Object library and Public API interface
ly_add_target(
    NAME MiniAudio ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        miniaudio_shared_files.cmake
        ${pal_dir}/miniaudio_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::MiniAudio.API
        PRIVATE
            Gem::MiniAudio.Private.Object
)

# By default, we will specify that the above target MiniAudio would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME MiniAudio.Clients NAMESPACE Gem TARGETS Gem::MiniAudio)
ly_create_alias(NAME MiniAudio.Servers NAMESPACE Gem TARGETS Gem::MiniAudio)

# For the Client and Server variants of MiniAudio Gem, an alias to the MiniAudio.API target will be made
ly_create_alias(NAME MiniAudio.Clients.API NAMESPACE Gem TARGETS Gem::MiniAudio.API)
ly_create_alias(NAME MiniAudio.Servers.API NAMESPACE Gem TARGETS Gem::MiniAudio.API)

# If we are on a host platform, we want to add the host tools targets like the MiniAudio.Editor MODULE target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The MiniAudio.Editor.API target can be used by other gems that want to interact with the MiniAudio.Editor module
    ly_add_target(
        NAME MiniAudio.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            miniaudio_editor_api_files.cmake
            ${pal_dir}/miniaudio_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The MiniAudio.Editor.Private.Object target is an internal target
    # which is only to be used by this gems CMakeLists.txt and any subdirectories
    # Other gems should not use this target
    ly_add_target(
        NAME MiniAudio.Editor.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            miniaudio_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                $<TARGET_OBJECTS:Gem::MiniAudio.Private.Object>
            PRIVATE
                 3rdParty::miniaudio
                 3rdParty::stb_vorbis
    )

    ly_add_target(
        NAME MiniAudio.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            miniaudio_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::MiniAudio.Editor.API
            PRIVATE
                Gem::MiniAudio.Editor.Private.Object
    )

    # By default, we will specify that the above target MiniAudio would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME MiniAudio.Tools    NAMESPACE Gem TARGETS Gem::MiniAudio.Editor)
    ly_create_alias(NAME MiniAudio.Builders NAMESPACE Gem TARGETS Gem::MiniAudio.Editor)

    # For the Tools and Builders variants of MiniAudio Gem, an alias to the MiniAudio.Editor API target will be made
    ly_create_alias(NAME MiniAudio.Tools.API NAMESPACE Gem TARGETS Gem::MiniAudio.Editor.API)
    ly_create_alias(NAME MiniAudio.Builders.API NAMESPACE Gem TARGETS Gem::MiniAudio.Editor.API)

endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for MiniAudio.Tests
    if(PAL_TRAIT_MINIAUDIO_TEST_SUPPORTED)
        # We support MiniAudio.Tests on this platform, add dependency on the Private Object target
        ly_add_target(
            NAME MiniAudio.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                miniaudio_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::MiniAudio.Private.Object
        )

        # Add MiniAudio.Tests to googletest
        ly_add_googletest(
            NAME Gem::MiniAudio.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_MINIAUDIO_EDITOR_TEST_SUPPORTED)
            # We support MiniAudio.Editor.Tests on this platform, add MiniAudio.Editor.Tests target which depends on
            # private MiniAudio.Editor.Private.Object target
            ly_add_target(
                NAME MiniAudio.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    miniaudio_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::MiniAudio.Private.Object
            )

            # Add MiniAudio.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::MiniAudio.Editor.Tests
            )
        endif()
    endif()
endif()

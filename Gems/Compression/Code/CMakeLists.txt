# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/Compression/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/Compression/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# The Compression.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME Compression.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        compression_api_files.cmake
        ${pal_dir}/compression_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
)

# Add the Compression.Private.Object target
# Note: We include the common files and the platform specific files which are set in
# 1.compression_private_files.cmake
# 2.${pal_dir}/compression_private_files.cmake
ly_add_target(
    NAME Compression.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        compression_private_files.cmake
        ${pal_dir}/compression_private_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

# Here add Compression target, it depends on the Private Object library and Public API interface
ly_add_target(
    NAME Compression ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        compression_shared_files.cmake
        ${pal_dir}/compression_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::Compression.API
        PRIVATE
            Gem::Compression.Private.Object
)

# By default, we will specify that the above target Compression would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME Compression.Clients NAMESPACE Gem TARGETS Gem::Compression)
ly_create_alias(NAME Compression.Servers NAMESPACE Gem TARGETS Gem::Compression)
ly_create_alias(NAME Compression.Unified NAMESPACE Gem TARGETS Gem::Compression)

# For the Client and Server variants of Compression Gem, an alias to the Compression.API target will be made
ly_create_alias(NAME Compression.Clients.API NAMESPACE Gem TARGETS Gem::Compression.API)
ly_create_alias(NAME Compression.Servers.API NAMESPACE Gem TARGETS Gem::Compression.API)
ly_create_alias(NAME Compression.Unified.API NAMESPACE Gem TARGETS Gem::Compression.API)

# If we are on a host platform, we want to add the host tools targets like the Compression.Editor MODULE target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The Compression.Editor.API target can be used by other gems that want to interact with the Compression.Editor module
    ly_add_target(
        NAME Compression.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            compression_editor_api_files.cmake
            ${pal_dir}/compression_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The Compression.Editor.Private.Object target is an internal target
    # which is only to be used by this Gem's CMakeLists.txt and any Subdirectories
    # Other Gems should not use this target
    ly_add_target(
        NAME Compression.Editor.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            compression_editor_private_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                $<TARGET_OBJECTS:Gem::Compression.Private.Object>
    )

    ly_add_target(
        NAME Compression.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            compression_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::Compression.Editor.API
            PRIVATE
                Gem::Compression.Editor.Private.Object
    )

    # By default, we will specify that the above target Compression would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME Compression.Tools    NAMESPACE Gem TARGETS Gem::Compression.Editor)
    ly_create_alias(NAME Compression.Builders NAMESPACE Gem TARGETS Gem::Compression.Editor)

    # For the Tools and Builders variants of Compression Gem, an alias to the Compression.Editor API target will be made
    ly_create_alias(NAME Compression.Tools.API NAMESPACE Gem TARGETS Gem::Compression.Editor.API)
    ly_create_alias(NAME Compression.Builders.API NAMESPACE Gem TARGETS Gem::Compression.Editor.API)

endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for Compression.Tests
    if(PAL_TRAIT_COMPRESSION_TEST_SUPPORTED)
        # We support Compression.Tests on this platform, add dependency on the Private Object target
        ly_add_target(
            NAME Compression.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                compression_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    Include
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::Compression.Private.Object
        )

        ly_add_googletest(
            NAME Gem::Compression.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_COMPRESSION_EDITOR_TEST_SUPPORTED)
            # We support Compression.Editor.Tests on this platform, add Compression.Editor.Tests target which depends on
            # private Compression.Editor.Private.Object target
            ly_add_target(
                NAME Compression.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    compression_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                        Include
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::Compression.Editor.Private.Object
            )

            ly_add_googletest(
                NAME Gem::Compression.Editor.Tests
            )
        endif()
    endif()
endif()

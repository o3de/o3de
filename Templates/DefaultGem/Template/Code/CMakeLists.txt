# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/${Name}/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/${Name}/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# The ${Name}.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME ${Name}.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        ${NameLower}_api_files.cmake
        ${pal_dir}/${NameLower}_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
)

# Add the ${Name}.Private.Object target
# Note: We include the common files and the platform specific files which are set in
# 1.${NameLower}_private_files.cmake
# 2.${pal_dir}/${NameLower}_private_files.cmake
ly_add_target(
    NAME ${Name}.Private.Object OBJECT
    NAMESPACE Gem
    FILES_CMAKE
        ${NameLower}_private_files.cmake
        ${pal_dir}/${NameLower}_private_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

# Here add ${Name} target, it depends on the Private Object library and Public API interface
ly_add_target(
    NAME ${Name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        ${NameLower}_shared_files.cmake
        ${pal_dir}/${NameLower}_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::${Name}.API
        PRIVATE
            Gem::${Name}.Private.Object
)

# By default, we will specify that the above target ${Name} would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ${Name}.Clients NAMESPACE Gem TARGETS Gem::${Name})
ly_create_alias(NAME ${Name}.Servers NAMESPACE Gem TARGETS Gem::${Name})

# For the Client and Server variants of ${Name} Gem, an alias to the ${Name}.API target will be made
ly_create_alias(NAME ${Name}.Clients.API NAMESPACE Gem TARGETS Gem::${Name}.API)
ly_create_alias(NAME ${Name}.Servers.API NAMESPACE Gem TARGETS Gem::${Name}.API)

# If we are on a host platform, we want to add the host tools targets like the ${Name}.Editor MODULE target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The ${Name}.Editor.API target can be used by other gems that want to interact with the ${Name}.Editor module
    ly_add_target(
        NAME ${Name}.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            ${NameLower}_editor_api_files.cmake
            ${pal_dir}/${NameLower}_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The ${Name}.Editor.Private.Object target is an internal target
    # which is only to be used by this Gem's CMakeLists.txt and any Subdirectories
    # Other Gems should not use this target
    ly_add_target(
        NAME ${Name}.Editor.Private.Object OBJECT
        NAMESPACE Gem
        FILES_CMAKE
            ${NameLower}_editor_private_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                $<TARGET_OBJECTS:Gem::${Name}.Private.Object>
    )

    ly_add_target(
        NAME ${Name}.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            ${NameLower}_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::${Name}.Editor.API
            PRIVATE
                Gem::${Name}.Editor.Private.Object
    )

    # By default, we will specify that the above target ${Name} would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME ${Name}.Tools    NAMESPACE Gem TARGETS Gem::${Name}.Editor)
    ly_create_alias(NAME ${Name}.Builders NAMESPACE Gem TARGETS Gem::${Name}.Editor)

    # For the Tools and Builders variants of ${Name} Gem, an alias to the ${Name}.Editor API target will be made
    ly_create_alias(NAME ${Name}.Tools.API NAMESPACE Gem TARGETS Gem::${Name}.Editor.API)
    ly_create_alias(NAME ${Name}.Builders.API NAMESPACE Gem TARGETS Gem::${Name}.Editor.API)

endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ${Name}.Tests
    if(PAL_TRAIT_${NameUpper}_TEST_SUPPORTED)
        # We support ${Name}.Tests on this platform, add dependency on the Private Object target
        ly_add_target(
            NAME ${Name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                ${NameLower}_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::${Name}.Private.Object
        )

        # Add ${Name}.Tests to googletest
        ly_add_googletest(
            NAME Gem::${Name}.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_${NameUpper}_EDITOR_TEST_SUPPORTED)
            # We support ${Name}.Editor.Tests on this platform, add ${Name}.Editor.Tests target which depends on
            # private ${Name}.Editor.Private.Object target
            ly_add_target(
                NAME ${Name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    ${NameLower}_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::${Name}.Private.Object
            )

            # Add ${Name}.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::${Name}.Editor.Tests
            )
        endif()
    endif()
endif()

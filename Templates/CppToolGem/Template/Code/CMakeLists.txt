# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/${Name}/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/${Name}/Code
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the ${Name}.Static target
# Note: We include the common files and the platform specific files which are set in ${NameLower}_common_files.cmake
# and in ${pal_dir}/${NameLower}_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME ${Name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        ${NameLower}_files.cmake
        ${pal_dir}/${NameLower}_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

# Here add ${Name} target, it depends on the ${Name}.Static
ly_add_target(
    NAME ${Name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        ${NameLower}_shared_files.cmake
        ${pal_dir}/${NameLower}_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${Name}.Static
)

# By default, we will specify that the above target ${Name} would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ${Name}.Clients NAMESPACE Gem TARGETS Gem::${Name})
ly_create_alias(NAME ${Name}.Servers NAMESPACE Gem TARGETS Gem::${Name})

# If we are on a host platform, we want to add the host tools targets like the ${Name}.Editor target which
# will also depend on ${Name}.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ${Name}.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTORCC
        FILES_CMAKE
            ${NameLower}_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::${Name}.Static
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
                Gem::${Name}.Editor.Static
    )

    # By default, we will specify that the above target ${Name} would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME ${Name}.Tools    NAMESPACE Gem TARGETS Gem::${Name}.Editor)
    ly_create_alias(NAME ${Name}.Builders NAMESPACE Gem TARGETS Gem::${Name}.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ${Name}.Static
    if(PAL_TRAIT_${NameUpper}_TEST_SUPPORTED)
        # We support ${Name}.Tests on this platform, add ${Name}.Tests target which depends on ${Name}.Static
        ly_add_target(
            NAME ${Name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                ${NameLower}_files.cmake
                ${NameLower}_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::${Name}.Static
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
            # We support ${Name}.Editor.Tests on this platform, add ${Name}.Editor.Tests target which depends on ${Name}.Editor
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
                        Gem::${Name}.Editor
            )

            # Add ${Name}.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::${Name}.Editor.Tests
            )
        endif()
    endif()
endif()

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


# If we are on a host platform, we want to add the host tools targets like the ${Name}.Editor target which
# will also depend on ${Name}.Editor.API target
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
        AUTORCC
        FILES_CMAKE
            ${NameLower}_editor_private_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
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
        RUNTIME_DEPENDENCIES
            Gem::QtForPython.Editor
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
    # We globally support tests, see if we support tests on this platform for ${Name}.Editor.Tests

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
    endif()
endif()

# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

# NOTE:To hook the pre-built binaries into the CMake targets, the binaries must be placed
# at the IMPORTED_LOCATION property specified in the Code/Platform/${PAL_PLATFORM_NAME}/${NameLower}*.cmake files

# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL.cmake)

# Check to see if the building the Gem Modules are supported for the current platform
if(NOT PAL_TRAIT_${NameUpper}_SUPPORTED)
    return()
endif()


# The ${gem_name}.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME ${gem_name}.API INTERFACE
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
    TARGET_PROPERTIES
)


# The ${gem_name} target is a prebuilt module that should contain extern "C" functions to hook into the Gem system
# The list of those functions are available at https://github.com/o3de/o3de/blob/development/Code/Framework/AzCore/AzCore/Module/DynamicModuleHandle.h#L101-L137
ly_add_target(
    NAME ${gem_name} IMPORTED ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/${LY_BUILD_PERMUTATION}/${NameLower}.cmake
    COMPILE_DEFINITIONS
        INTERFACE
    INCLUDE_DIRECTORIES
        INTERFACE
    BUILD_DEPENDENCIES
        INTERFACE
    RUNTIME_DEPENDENCIES
    TARGET_PROPERTIES
)

# By default, we will specify that the above target ${gem_name} would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

# For the Client and Server variants of ${gem_name} Gem, an alias to the ${gem_name}.API target will be made
ly_create_alias(NAME ${gem_name}.Clients.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Servers.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Unified.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)

# Add in CMake dependencies for each gem dependency listed in this gem's gem.json file
# for the Client, Server and Unified gem variants
o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Client Server Unified)

# If we are on a host platform, we want to add the host tools targets like the ${gem_name}.Tools and ${gem_name}.Builders
if (PAL_TRAIT_BUILD_HOST_TOOLS)
    # The ${gem_name}.Tools.API target can be used by other gems that want to interact with the ${gem_name}.Tools module
    ly_add_target(
        NAME ${gem_name}.Tools.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            ${NameLower}_tools_api_files.cmake
            ${pal_dir}/${NameLower}_tools_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
        TARGET_PROPERTIES
    )

    # The ${gem_name}.Tools target is a prebuilt module that should contain extern "C" functions to hook into the Gem system
    # The list of those functions are available at https://github.com/o3de/o3de/blob/development/Code/Framework/AzCore/AzCore/Module/DynamicModuleHandle.h#L101-L137
    ly_add_target(
        NAME ${gem_name}.Tools IMPORTED GEM_MODULE
        NAMESPACE Gem
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/${LY_BUILD_PERMUTATION}/${NameLower}_tools.cmake
        COMPILE_DEFINITIONS
            INTERFACE
        INCLUDE_DIRECTORIES
            INTERFACE
        RUNTIME_DEPENDENCIES
        TARGET_PROPERTIES
    )

    # The Builders moudles are loaded by the AssetProcessor, AssetBuilder and AssetBundler
    # The ${gem_name}.Builders.API target can be used by other gems that want to interact with the ${gem_name}.Builders module
    ly_add_target(
        NAME ${gem_name}.Builders.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            ${NameLower}_builders_api_files.cmake
            ${pal_dir}/${NameLower}_builders_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
        TARGET_PROPERTIES
    )

    # The ${gem_name}.Tools target is a prebuilt module that should contain extern "C" functions to hook into the Gem system
    # The list of those functions are available at https://github.com/o3de/o3de/blob/development/Code/Framework/AzCore/AzCore/Module/DynamicModuleHandle.h#L101-L137
    ly_add_target(
        NAME ${gem_name}.Builders IMPORTED GEM_MODULE
        NAMESPACE Gem
        PLATFORM_INCLUDE_FILES
            ${pal_dir}/${LY_BUILD_PERMUTATION}/${NameLower}_builders.cmake
        COMPILE_DEFINITIONS
            INTERFACE
        INCLUDE_DIRECTORIES
            INTERFACE
        RUNTIME_DEPENDENCIES
        TARGET_PROPERTIES
    )

    # Add in CMake dependencies for each gem dependency listed in this gem's gem.json file
    # for the Tools and Builders gem variants
    o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Tools Builders)
endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # Check if test are supported on the current platform
    if(PAL_TRAIT_${NameUpper}_TEST_SUPPORTED)
        ly_add_target(
            NAME ${gem_name}.Tests IMPORTED ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            PLATFORM_INCLUDE_FILES
                ${pal_dir}/${LY_BUILD_PERMUTATION}/${NameLower}_tests.cmake
            COMPILE_DEFINITIONS
                INTERFACE
            INCLUDE_DIRECTORIES
                INTERFACE
            TARGET_PROPERTIES
        )
        ly_add_googletest(
            NAME Gem::${gem_name}.Tests
    )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        if(PAL_TRAIT_${NameUpper}_EDITOR_TEST_SUPPORTED)
            # Check if the tools test are supported on the current platform
            ly_add_target(
                NAME ${gem_name}.Tools.Tests IMPORTED ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                PLATFORM_INCLUDE_FILES
                    ${pal_dir}/${LY_BUILD_PERMUTATION}/${NameLower}_tools_tests.cmake
                COMPILE_DEFINITIONS
                    INTERFACE
                INCLUDE_DIRECTORIES
                    INTERFACE
                TARGET_PROPERTIES
            )

            ly_add_googletest(
                NAME Gem::${gem_name}.Tools.Tests
            )
        endif()
    endif()
endif()

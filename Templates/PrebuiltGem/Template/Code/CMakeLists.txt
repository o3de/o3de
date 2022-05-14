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
    TARGET_PROPERTIES
)


# The ${Name} target is a prebuilt module that should contain extern "C" functions to hook into the Gem system
# The list of those functions are available at https://github.com/o3de/o3de/blob/development/Code/Framework/AzCore/AzCore/Module/DynamicModuleHandle.h#L101-L137
ly_add_target(
    NAME ${Name} IMPORTED ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
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

# By default, we will specify that the above target ${Name} would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ${Name}.Clients NAMESPACE Gem TARGETS Gem::${Name})
ly_create_alias(NAME ${Name}.Servers NAMESPACE Gem TARGETS Gem::${Name})

# For the Client and Server variants of ${Name} Gem, an alias to the ${Name}.API target will be made
ly_create_alias(NAME ${Name}.Clients.API NAMESPACE Gem TARGETS Gem::${Name}.API)
ly_create_alias(NAME ${Name}.Servers.API NAMESPACE Gem TARGETS Gem::${Name}.API)

# If we are on a host platform, we want to add the host tools targets like the ${Name}.Tools and ${Name}.Builders
if (PAL_TRAIT_BUILD_HOST_TOOLS)
    # The ${Name}.Tools.API target can be used by other gems that want to interact with the ${Name}.Tools module
    ly_add_target(
        NAME ${Name}.Tools.API INTERFACE
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

    # The ${Name}.Tools target is a prebuilt module that should contain extern "C" functions to hook into the Gem system
    # The list of those functions are available at https://github.com/o3de/o3de/blob/development/Code/Framework/AzCore/AzCore/Module/DynamicModuleHandle.h#L101-L137
    ly_add_target(
        NAME ${Name}.Tools IMPORTED GEM_MODULE
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
    # The ${Name}.Builders.API target can be used by other gems that want to interact with the ${Name}.Builders module
    ly_add_target(
        NAME ${Name}.Builders.API INTERFACE
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

    # The ${Name}.Tools target is a prebuilt module that should contain extern "C" functions to hook into the Gem system
    # The list of those functions are available at https://github.com/o3de/o3de/blob/development/Code/Framework/AzCore/AzCore/Module/DynamicModuleHandle.h#L101-L137
    ly_add_target(
        NAME ${Name}.Builders IMPORTED GEM_MODULE
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
endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # Check if test are supported on the current platform
    if(PAL_TRAIT_${NameUpper}_TEST_SUPPORTED)
        ly_add_target(
            NAME ${Name}.Tests IMPORTED ${PAL_TRAIT_TEST_TARGET_TYPE}
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
            NAME Gem::${Name}.Tests
    )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        if(PAL_TRAIT_${NameUpper}_EDITOR_TEST_SUPPORTED)
            # Check if the tools test are supported on the current platform
            ly_add_target(
                NAME ${Name}.Tools.Tests IMPORTED ${PAL_TRAIT_TEST_TARGET_TYPE}
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
                NAME Gem::${Name}.Tools.Tests
            )
        endif()
    endif()
endif()

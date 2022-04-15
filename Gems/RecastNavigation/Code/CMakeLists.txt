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
#       i.e. It could here in our gem : Gems/RecastNavigation/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/RecastNavigation/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_name}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add RecastNavigation dependency using FetchContent
# Note: RecastNavigation::Detour requires lowering of the warning level to compile.
include(FetchContent)
FetchContent_Declare(
        RecastNavigation
        GIT_REPOSITORY "https://github.com/recastnavigation/recastnavigation"
        GIT_TAG "5a870d427e47abd4a8e4ce58a95582ec049434d5" # main branch
)
set(RECASTNAVIGATION_DEMO OFF)
set(RECASTNAVIGATION_TESTS OFF)
set(RECASTNAVIGATION_EXAMPLES OFF)
# Lower warning level for RecastNavigation::Detour library
get_property(
    compile_options
    DIRECTORY
    PROPERTY COMPILE_OPTIONS
)
set_property(
    DIRECTORY
    APPEND
    PROPERTY COMPILE_OPTIONS /W3
)
FetchContent_MakeAvailable(RecastNavigation)
# Restore compile options
set_property(
    DIRECTORY
    PROPERTY COMPILE_OPTIONS ${compile_options}
)
unset(compile_options)

# Add the RecastNavigation.Static target
# Note: We include the common files and the platform specific files which are set in recastnavigation_common_files.cmake
# and in ${pal_dir}/recastnavigation_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME RecastNavigation.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        recastnavigation_files.cmake
        ${pal_dir}/recastnavigation_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            RecastNavigation::DebugUtils
            RecastNavigation::Detour
            RecastNavigation::Recast
)

# Here add RecastNavigation target, it depends on the RecastNavigation.Static
ly_add_target(
    NAME RecastNavigation ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        recastnavigation_shared_files.cmake
        ${pal_dir}/recastnavigation_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::RecastNavigation.Static
)

# By default, we will specify that the above target RecastNavigation would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME RecastNavigation.Clients NAMESPACE Gem TARGETS Gem::RecastNavigation)
ly_create_alias(NAME RecastNavigation.Servers NAMESPACE Gem TARGETS Gem::RecastNavigation)
ly_create_alias(NAME RecastNavigation.Tools    NAMESPACE Gem TARGETS Gem::RecastNavigation)
ly_create_alias(NAME RecastNavigation.Builders NAMESPACE Gem TARGETS Gem::RecastNavigation)

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for RecastNavigation.Static
    if(PAL_TRAIT_RECASTNAVIGATION_TEST_SUPPORTED)
        # We support RecastNavigation.Tests on this platform, add RecastNavigation.Tests target which depends on RecastNavigation.Static
        ly_add_target(
            NAME RecastNavigation.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                recastnavigation_files.cmake
                recastnavigation_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::RecastNavigation.Static
        )

        # Add RecastNavigation.Tests to googletest
        ly_add_googletest(
            NAME Gem::RecastNavigation.Tests
        )
    endif()
endif()

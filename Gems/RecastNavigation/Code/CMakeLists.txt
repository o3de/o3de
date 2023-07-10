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
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

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
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(RECASTNAVIGATION_DEMO OFF)
set(RECASTNAVIGATION_TESTS OFF)
set(RECASTNAVIGATION_EXAMPLES OFF)
get_property(
    compile_options
    DIRECTORY
    PROPERTY COMPILE_OPTIONS
)
include(${pal_dir}/PAL_recast_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Include compiler settings for Recast libraries
set(common_platform_dir "${CMAKE_CURRENT_LIST_DIR}/Platform/Common")
include(${common_platform_dir}/${PAL_TRAIT_COMPILER_ID}/PAL_recast_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake)

set_property(
    DIRECTORY
    APPEND
    PROPERTY COMPILE_DEFINITIONS DT_POLYREF64
)
FetchContent_MakeAvailable(RecastNavigation)
# Restore compile options
set_property(
    DIRECTORY
    PROPERTY COMPILE_OPTIONS ${compile_options}
)
unset(compile_options)

# Place external recast libraries under RecastNavigation gem.
get_property(recastnavigation_gem_root GLOBAL PROPERTY "@GEMROOT:RecastNavigation@")
ly_get_engine_relative_source_dir(${recastnavigation_gem_root} relative_recastnavigation_gem_root)
set(recastLibraries DebugUtils;Detour;DetourCrowd;DetourTileCache;Recast)
foreach(recastLibrary ${recastLibraries})
    set_property(TARGET ${recastLibrary} PROPERTY FOLDER "${relative_recastnavigation_gem_root}/External")
endforeach()

# Export external recast navigation targets for installers.
set_property(DIRECTORY APPEND PROPERTY O3DE_SUBDIRECTORY_INSTALL_CODE [[
    set(recastLibraries DebugUtils;Detour;DetourCrowd;DetourTileCache;Recast)
    foreach(recastLibrary ${recastLibraries})
        add_library(RecastNavigation::${recastLibrary} STATIC IMPORTED GLOBAL)

        set_target_properties(RecastNavigation::${recastLibrary} 
            PROPERTIES 
                IMPORTED_LOCATION 
                    "${LY_ROOT_FOLDER}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${recastLibrary}${CMAKE_STATIC_LIBRARY_SUFFIX}")

        target_compile_definitions(RecastNavigation::${recastLibrary} 
            INTERFACE 
                DT_POLYREF64)

        ly_target_include_system_directories(TARGET RecastNavigation::${recastLibrary} 
            INTERFACE 
                "${LY_ROOT_FOLDER}/include/recastnavigation")
    endforeach()
]]
)

# The RecastNavigation.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME RecastNavigation.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        recastnavigation_api_files.cmake
        ${pal_dir}/recastnavigation_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
            AZ::AzCore
            RecastNavigation::Detour
            RecastNavigation::Recast
)

# The RecastNavigation.Private.Object target is an internal target
# It should not be used outside of this Gems CMakeLists.txt
ly_add_target(
    NAME RecastNavigation.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        recastnavigation_private_files.cmake
        ${pal_dir}/recastnavigation_private_files.cmake
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
            RecastNavigation::DebugUtils
            RecastNavigation::Detour
            RecastNavigation::Recast
            Gem::LmbrCentral.Static
            Gem::DebugDraw.API
)

# Here add RecastNavigation target, it depends on the Private Object library and Public API interface
ly_add_target(
    NAME RecastNavigation ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        recastnavigation_shared_files.cmake
        ${pal_dir}/recastnavigation_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::RecastNavigation.API
        PRIVATE
            Gem::RecastNavigation.Private.Object
)

# By default, we will specify that the above target RecastNavigation would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME RecastNavigation.Clients NAMESPACE Gem TARGETS Gem::RecastNavigation)
ly_create_alias(NAME RecastNavigation.Servers NAMESPACE Gem TARGETS Gem::RecastNavigation)
ly_create_alias(NAME RecastNavigation.Unified NAMESPACE Gem TARGETS Gem::RecastNavigation)

# For the Client and Server variants of RecastNavigation Gem, an alias to the RecastNavigation.API target will be made
ly_create_alias(NAME RecastNavigation.Clients.API NAMESPACE Gem TARGETS Gem::RecastNavigation.API)
ly_create_alias(NAME RecastNavigation.Servers.API NAMESPACE Gem TARGETS Gem::RecastNavigation.API)
ly_create_alias(NAME RecastNavigation.Unified.API NAMESPACE Gem TARGETS Gem::RecastNavigation.API)

# If we are on a host platform, we want to add the host tools targets like the RecastNavigation.Editor MODULE target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The RecastNavigation.Editor.API target can be used by other gems that want to interact with the RecastNavigation.Editor module
    ly_add_target(
        NAME RecastNavigation.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            recastnavigation_editor_api_files.cmake
            ${pal_dir}/recastnavigation_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The RecastNavigation.Editor.Private.Object target is an internal target
    # which is only to be used by this gems CMakeLists.txt and any subdirectories
    # Other gems should not use this target
    ly_add_target(
        NAME RecastNavigation.Editor.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            recastnavigation_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::RecastNavigation.Private.Object
                Gem::LmbrCentral.Static
                Gem::DebugDraw.API
    )

    ly_add_target(
        NAME RecastNavigation.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            recastnavigation_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::RecastNavigation.Editor.API
            PRIVATE
                Gem::RecastNavigation.Editor.Private.Object
    )

    # By default, we will specify that the above target RecastNavigation would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME RecastNavigation.Tools    NAMESPACE Gem TARGETS Gem::RecastNavigation.Editor)
    ly_create_alias(NAME RecastNavigation.Builders NAMESPACE Gem TARGETS Gem::RecastNavigation.Editor)

    # For the Tools and Builders variants of RecastNavigation Gem, an alias to the RecastNavigation.Editor API target will be made
    ly_create_alias(NAME RecastNavigation.Tools.API NAMESPACE Gem TARGETS Gem::RecastNavigation.Editor.API)
    ly_create_alias(NAME RecastNavigation.Builders.API NAMESPACE Gem TARGETS Gem::RecastNavigation.Editor.API)

endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for RecastNavigation.Tests
    if(PAL_TRAIT_RECASTNAVIGATION_TEST_SUPPORTED)
        # We support RecastNavigation.Tests on this platform, add dependency on the Private Object target
        ly_add_target(
            NAME RecastNavigation.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                recastnavigation_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::RecastNavigation.Private.Object
                    Gem::RecastNavigation.API
                    Gem::LmbrCentral.API
                    Gem::PhysX.Mocks
        )

        # Add RecastNavigation.Tests to googletest
        ly_add_googletest(
            NAME Gem::RecastNavigation.Tests
            LABELS REQUIRES_tiaf
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_RECASTNAVIGATION_EDITOR_TEST_SUPPORTED)
            # We support RecastNavigation.Editor.Tests on this platform, add RecastNavigation.Editor.Tests target which depends on
            # private RecastNavigation.Editor.Private.Object target
            ly_add_target(
                NAME RecastNavigation.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    recastnavigation_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        AZ::AzToolsFramework
                        Gem::RecastNavigation.Private.Object
                        Gem::RecastNavigation.Editor.Private.Object
                        Gem::RecastNavigation.API
                        Gem::LmbrCentral.API
                        Gem::PhysX.Mocks
            )

            # Add RecastNavigation.Editor.Tests to googletest
            # Commented out as currently there are no RecastNavigation Editor tests
            # ly_add_googletest(
                # NAME Gem::RecastNavigation.Editor.Tests
            # )
        endif()
    endif()
endif()

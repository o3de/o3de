#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Always start by checking if the target already exists.
# This prevents repeated calls but also allows the user to substitute their own 3rd party library
# if they wish to do so.

if (TARGET 3rdParty::ogg)
    return()
endif()

function(Getogg)
    # Variables inside a local function are scoped to the function body.
    # Putting all of this inside a function lets us basically ensure that any variables set by the
    # external 3rdParty CMake file do not have any effect on the outside world.
    # and allows us not to have to save and restore anything except variables that escape scope like CACHE variables.
    # Part 1:  Where do you get the library from?  Make sure to inform the user of the source of the library and any patches applied.
    include(FetchContent)

    set(ogg_GIT_REPO "https://github.com/xiph/ogg.git")
    set(ogg_GIT_TAG "v1.3.6")
    set(ogg_GIT_HASH "be05b13e98b048f0b5a0f5fa8ce514d56db5f822") # Better to use pinned hashes for security.

    FetchContent_Declare(
            ogg
            GIT_REPOSITORY ${ogg_GIT_REPO}
            GIT_TAG ${ogg_GIT_HASH}
            GIT_SHALLOW TRUE
            EXCLUDE_FROM_ALL # prevent it from executing its install commands.
    )

    # please always be really clear about what third parties your gem uses.
    message(STATUS "MiniAudio Gem uses ${ogg_GIT_REPO} ${ogg_GIT_TAG} (BSD-3-Clause)")

    # Part 2: Set the build settings and trigger the actual execution of the downloaded CMakeLists.txt file
    # Note that CMAKE_ARGS does NOT WORK for FetchContent_*, only ExternalProject.
    # Thus, you must set any configuration settings here, in the scope in which you call FetchContent_MakeAvailable.
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_FRAMEWORK OFF)
    set(INSTALL_CMAKE_PACKAGE_MODULE OFF)
    set(INSTALL_DOCS OFF)
    set(INSTALL_PKG_CONFIG_MODULE OFF)
    set(BUILD_TESTING OFF)

    # the below line is what actualy runs its CMakeList.txt file and executes targets and so on:
    FetchContent_MakeAvailable(ogg)
    
    # restore any CACHE settings changed:
    set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)
endfunction()

Getogg()

# for extra safety, we'll remove the function from the global scope, so that it can't be called again.
unset(Getogg)

get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)
o3de_fixup_fetchcontent_targets(
    IDE_FOLDER "${relative_this_gem_root}/External" 
    TARGETS ogg)

# Copy headers and license files, as well as a custom "find" file that declares the targets as IMPORTED
FetchContent_GetProperties(ogg SOURCE_DIR ogg_source_dir)
ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findogg.cmake DESTINATION cmake/3rdParty)
ly_install(DIRECTORY ${ogg_source_dir}/include/ogg DESTINATION include/ogg COMPONENT CORE)
ly_install(FILES ${ogg_source_dir}/COPYING DESTINATION include/ogg COMPONENT CORE)

# signal that find_package(ogg) has succeeded.
# we have to set it on the PARENT_SCOPE since we're in a function
set(ogg_FOUND TRUE PARENT_SCOPE)

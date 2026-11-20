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

block()
    # Variables inside a local function are scoped to the function body.
    # Putting all of this inside a function lets us basically ensure that any variables set by the
    # external 3rdParty CMake file do not have any effect on the outside world.
    # and allows us not to have to save and restore anything except variables that escape scope like CACHE variables.
    # Part 1:  Where do you get the library from?  Make sure to inform the user of the source of the library and any patches applied.
    o3de_fetch_content(ogg
        VERSION "v1.3.6"
        LICENSE "BSD-3-Clause"
        URL "https://github.com/xiph/ogg/archive/refs/tags/v1.3.6.tar.gz"
        URL_HASH "95b643da661155d79db9de2fca55daed3a8d491039829def246aacb3d9201c81"
        GIT "https://github.com/xiph/ogg.git"
        GIT_HASH "be05b13e98b048f0b5a0f5fa8ce514d56db5f822"
        EXCLUDE_FROM_ALL
    )

    # Part 2: Set the build settings and trigger the actual execution of the downloaded CMakeLists.txt file
    # Note that CMAKE_ARGS does NOT WORK for FetchContent_*, only ExternalProject.
    # Thus, you must set any configuration settings here, in the scope in which you call FetchContent_MakeAvailable.
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
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
endblock()

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

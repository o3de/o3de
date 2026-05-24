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

if (TARGET 3rdParty::vorbis)
    return()
endif()

# vorbis depends on Ogg.
find_package(Ogg)

function(Getvorbis)
    # we intentionally open a new function scope here so that all variable SET is temporary and is discarded after the function ends.
    # this means we can set variables and when this function ends we only need to restore any variables that may have escaped the
    # scope, for example, CACHE variables.

    # Part 1:  Where do you get the library from?  Make sure to inform the user of the source of the library and any patches applied.
    include(FetchContent)

    set(vorbis_GIT_REPO "https://github.com/xiph/vorbis.git")
    set(vorbis_GIT_TAG "v1.3.7")
    set(vorbis_GIT_HASH "0657aee69dec8508a0011f47f3b69d7538e9d262")  # better to use hashes for security
    FetchContent_Declare(
            vorbis
            GIT_REPOSITORY ${vorbis_GIT_REPO}
            GIT_TAG ${vorbis_GIT_HASH}
            GIT_SHALLOW TRUE
            EXCLUDE_FROM_ALL # this actually stops it from running its `install` commands, we need to handle the layout ourselves.
    )

    # please always be really clear about what third parties your gem uses.
    message(STATUS "MiniAudio Gem uses ${vorbis_GIT_REPO} ${vorbis_GIT_TAG} (BSD-3-Clause)")

    # Part 2: Set the build settings and trigger the actual execution of the downloaded CMakeLists.txt file
    # Note that CMAKE_ARGS does NOT WORK for FetchContent_*, only ExternalProject.
    # Thus, you must set any configuration settings here, in the scope in which you call FetchContent_MakeAvailable.
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

    # The rest of these are all specific settings that come from vorbis CMakeLists.txt files.
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_FRAMEWORK OFF)
    set(INSTALL_CMAKE_PACKAGE_MODULE OFF)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
    set(BUILD_TESTING OFF)

    # the below line is what actualy runs its CMakeLists.txt file and executes targets and so on:
    FetchContent_MakeAvailable(vorbis)
    set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)
endfunction()

Getvorbis()
# for extra safety, we'll remove the function from the global scope, so that it can't be called again.
unset(Getvorbis)

# Vorbis will show up in IDEs underneath the gem that imported it first:
get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)
o3de_fixup_fetchcontent_targets(
    IDE_FOLDER "${relative_this_gem_root}/External" 
    TARGETS vorbis vorbisfile vorbisenc)

# Copy headers and license files, as well as a custom "find" file that declares the targets as IMPORTED
FetchContent_GetProperties(vorbis SOURCE_DIR vorbis_source_dir)
ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findvorbis.cmake DESTINATION cmake/3rdParty)
ly_install(DIRECTORY ${vorbis_source_dir}/include/vorbis DESTINATION include/vorbis COMPONENT CORE)
ly_install(FILES ${vorbis_source_dir}/COPYING DESTINATION include/vorbis COMPONENT CORE)

# using EXCLUDE_FROM_ALL actually removes all targets from the default dependency tree calculation
# when in project generation, meaning, unless something explicitly depends on these libraries, they won't
# end up making it into the final list of build targets.  So here, we manually name their dependencies to
# reconnect them into the build tree.
add_dependencies(vorbisfile vorbis)
add_dependencies(vorbis ogg)

# signal that find_package(vorbis) has succeeded.
# we have to set it on the PARENT_SCOPE since we're in a function
set(vorbis_FOUND TRUE PARENT_SCOPE)

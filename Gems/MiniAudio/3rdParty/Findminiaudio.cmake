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

if (TARGET 3rdParty::miniaudio)
    return()
endif()

# If you want to use opus or other similar, consider following this same pattern, that is,
# call find_package here, for it, and provide a `Find<PackageName>.cmake` file for it in this same folder that pretty much
# copies this structure.  O3DE doesn't currently ship with opus support as there are patent questions about it.
find_package(vorbis)

function(Getminiaudio)
    # Variables inside a local function are scoped to the function body.
    # Putting all of this inside a function lets us basically ensure that any variables set by us before invoking the
    # external 3rdParty CMake file do not have any effect on the outside world, and allows us not to have to save and restore anything
    # except for cache changes.

    # Part 1:  Where do you get the library from?  Make sure to inform the user of the source of the library and any patches applied.
    include(FetchContent)
    set(MINIAUDIO_GIT_REPO "https://github.com/mackron/miniaudio.git")
    set(MINIAUDIO_GIT_TAG "0.11.22")
    set(MINIAUDIO_GIT_HASH "350784a9467a79d0fa65802132668e5afbcf3777") # prefer to pin to hashes for security
    FetchContent_Declare(
            miniaudio
            GIT_REPOSITORY ${MINIAUDIO_GIT_REPO}
            GIT_TAG ${MINIAUDIO_GIT_HASH}
            GIT_SHALLOW TRUE
            # it is not necessary to EXCLUDE_FROM_ALL above because miniaudio does not try to force install anything.
    )
    message(STATUS "MiniAudio Gem uses ${MINIAUDIO_GIT_REPO} ${MINIAUDIO_GIT_TAG} (MIT No Attribution)")

    # Part 2: Set the build settings and trigger the actual execution of the downloaded CMakeLists.txt file
    # Note that CMAKE_ARGS does NOT WORK for FetchContent_*, only ExternalProject.
    # Thus, you must set any configuration settings here, in the scope in which you call FetchContent_MakeAvailable.
    # These settings will be applied only to the current CMake scope - so it is only worth saving and restoring values from settings
    # from the cache since this is in its own scope inside this function.
    set(CMAKE_MESSAGE_LOG_LEVEL ${O3DE_FETCHCONTENT_MESSAGE_LEVEL})
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

    # The rest of these are all specific settings that come from MioniAudio's CMakeLists.txt files.
    set(MINIAUDIO_BUILD_EXAMPLES OFF)
    set(MINIAUDIO_BUILD_TESTS OFF)
    set(BUILD_TESTING OFF)
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_FRAMEWORK OFF)
    set(INSTALL_CMAKE_PACKAGE_MODULE OFF)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

    # the below line is what actualy runs its CMakeList.txt file, recurses into its subfolder, defines targets and so on:
    FetchContent_MakeAvailable(miniaudio)

    set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)
endfunction()

Getminiaudio()
unset(Getminiaudio)

get_property(this_gem_root GLOBAL PROPERTY "@GEMROOT:${gem_name}@")
ly_get_engine_relative_source_dir(${this_gem_root} relative_this_gem_root)

set(MINIAUDIO_TARGETS 
        miniaudio
        miniaudio_channel_combiner_node
        miniaudio_channel_separator_node
        miniaudio_ltrim_node
        miniaudio_reverb_node
        miniaudio_vocoder_node
        miniaudio_libvorbis)  #if you intend to use opus, you will have to add it here.

o3de_fixup_fetchcontent_targets(
    IDE_FOLDER 
        "${relative_this_gem_root}/External" 
    TARGETS 
        ${MINIAUDIO_TARGETS})

# Copy headers and license files, as well as a custom "find" file that declares the targets as IMPORTED
FetchContent_GetProperties(miniaudio SOURCE_DIR miniaudio_source_dir)
ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findminiaudio.cmake DESTINATION cmake/3rdParty)
ly_install(FILES ${miniaudio_source_dir}/miniaudio.h DESTINATION include/miniaudio COMPONENT CORE)
ly_install(FILES ${miniaudio_source_dir}/LICENSE DESTINATION include/miniaudio COMPONENT CORE)

# plugin headers
foreach(node_plugin ma_channel_combiner_node ma_channel_separator_node ma_ltrim_node ma_reverb_node ma_vocoder_node)
    ly_install(
        FILES        "${miniaudio_source_dir}/extras/nodes/${node_plugin}/${node_plugin}.h"
        DESTINATION  "include/miniaudio/extras/nodes/${node_plugin}"
        COMPONENT    CORE)
endforeach()

# decoder headers
# note: Future proofed for if opus is added.
foreach(decoder_plugin libopus libvorbis)
    ly_install(
        FILES        "${miniaudio_source_dir}/extras/decoders/${decoder_plugin}/miniaudio_${decoder_plugin}.h"
        DESTINATION  "include/miniaudio/extras/decoders/${decoder_plugin}"
        COMPONENT    CORE)
        endforeach()

# using EXCLUDE_FROM_ALL actually removes all targets from the default dependency tree calculation
# when in project generation, meaning, unless something explicitly depends on these libraries, they won't
# end up making it into the final list of build targets.  So here, we manually name their dependencies to
# reconnect them into the build tree.  Note that O3DE adds a dependency between the gem, and `miniaudio`
# when it sees a dependency on 3rdParty::miniaudio, but miniaudio itself must declare dependencies on
# anything it uses, or else they would be pruned.

# if you use opus, you will have to add it here, if opus has the same issue about installing without being
# able to disable installation, and thus requiring EXCLUDE_FROM_ALL
add_dependencies(miniaudio_libvorbis vorbisfile vorbisenc)

# signal that find_package(MiniAudio) has succeeded.
# we have to set it on the PARENT_SCOPE since we're in a function
set(MiniAudio_FOUND TRUE PARENT_SCOPE)

# for extra safety, we'll remove the function from the global scope, so that it can't be called again.

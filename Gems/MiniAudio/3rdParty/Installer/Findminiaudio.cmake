#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This file is used in the INSTALLER version of O3DE.  
# This file is included in cmake/3rdParty, which is already part of the search path for Findxxxxx.cmake files.
if (TARGET 3rdParty::miniaudio)
    return()
endif()

# The default installer version of O3DE tries to make everything you need already included, but if you're making your own
# custom version of it, you can turn this off to trim dependencies and make it smaller.
set(MINIAUDIO_USE_VORBIS ON CACHE BOOL "Turn off to disable vobis support in MiniAudio gem (You must recompile the engine from source)")
if (MINIAUDIO_USE_VORBIS)
    find_package(vorbis)
endif()
# It is still worth notifying people that they are accepting a 3rd Party Library here, and what license it uses, and
# where to get it.
set(MINIAUDIO_GIT_REPO "https://github.com/mackron/miniaudio.git")
set(MINIAUDIO_GIT_TAG "0.11.22")
message(STATUS "MiniAudio Gem uses ${MINIAUDIO_GIT_REPO} ${MINIAUDIO_GIT_TAG} (MIT No Attribution)")

set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")

set(MINIAUDIO_TARGETS 
        miniaudio
        miniaudio_channel_combiner_node
        miniaudio_channel_separator_node
        miniaudio_ltrim_node
        miniaudio_reverb_node
        miniaudio_vocoder_node)

if (MINIAUDIO_USE_VORBIS)
    list(APPEND MINIAUDIO_TARGETS miniaudio_libvorbis)
endif()

foreach(MiniAudio_Target ${MINIAUDIO_TARGETS})
    add_library(${MiniAudio_Target} STATIC IMPORTED GLOBAL)
    set_target_properties(${MiniAudio_Target} PROPERTIES 
    IMPORTED_LOCATION         "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}${MiniAudio_Target}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_DEBUG   "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}${MiniAudio_Target}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE "${BASE_LIBRARY_FOLDER}/release/${CMAKE_STATIC_LIBRARY_PREFIX}${MiniAudio_Target}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ly_target_include_system_directories(TARGET ${MiniAudio_Target} INTERFACE "${LY_ROOT_FOLDER}/include/miniaudio")
    add_library(3rdParty::${MiniAudio_Target} ALIAS ${MiniAudio_Target})
endforeach()

target_link_libraries(
    miniaudio INTERFACE 
        miniaudio_channel_combiner_node 
        miniaudio_channel_separator_node
        miniaudio_ltrim_node
        miniaudio_reverb_node
        miniaudio_vocoder_node)

if (MINIAUDIO_USE_VORBIS)
    target_link_libraries(miniaudio INTERFACE miniaudio_libvorbis)
    target_link_libraries(miniaudio_libvorbis INTERFACE vorbisfile)
endif()

# notify O3DE That we have satisfied the MiniAudio find_package requirements.
set(miniaudio_FOUND TRUE)

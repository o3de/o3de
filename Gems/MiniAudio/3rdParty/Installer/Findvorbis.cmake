# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# This file is used in the INSTALLER version of O3DE.  
# This file is included in cmake/3rdParty, which is already part of the search path for Findxxxxx.cmake files.
if (TARGET 3rdParty::vorbis)
    return()
endif()

# vorbis depends on ogg
find_package(ogg)

# It is still worth notifying people that they are accepting a 3rd Party Library here, and what license it uses, and
# where to get it.
set(vorbis_GIT_REPO "https://github.com/xiph/vorbis.git")
set(vorbis_GIT_TAG "v1.3.7")
message(STATUS "MiniAudio Gem uses ${vorbis_GIT_REPO} ${vorbis_GIT_TAG} (BSD 3-Clause)")

set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")

set(VORBIS_TARGETS vorbis vorbisfile)
foreach(Vorbis_Target ${VORBIS_TARGETS})
    add_library(${Vorbis_Target} STATIC IMPORTED GLOBAL)
    set_target_properties(${Vorbis_Target} PROPERTIES 
    IMPORTED_LOCATION         "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}${Vorbis_Target}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_DEBUG   "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}${Vorbis_Target}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE "${BASE_LIBRARY_FOLDER}/release/${CMAKE_STATIC_LIBRARY_PREFIX}${Vorbis_Target}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ly_target_include_system_directories(TARGET ${Vorbis_Target} INTERFACE "${LY_ROOT_FOLDER}/include/miniaudio")
    add_library(3rdParty::${Vorbis_Target} ALIAS ${Vorbis_Target})
endforeach()

target_link_libraries(vorbisfile INTERFACE vorbis)
target_link_libraries(vorbis     INTERFACE ogg)

# notify O3DE That we have satisfied the vorbis find_package requirements.
set(vorbis_FOUND TRUE)


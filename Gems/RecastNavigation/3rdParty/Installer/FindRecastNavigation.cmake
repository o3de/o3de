#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::RecastNavigation::Recast)
    return()
endif()

# This file is used in the INSTALLER version of O3DE.
# This file is included in cmake/3rdParty, which is already part of the search path for Findxxxxx.cmake files.

# It is still worth notifying people that they are accepting a 3rd Party Library here, and what license it uses, and
# where to get it.
set(RECAST_GIT_REPO "https://github.com/recastnavigation/recastnavigation.git")
set(RECAST_GIT_TAG 5a870d4)
message(STATUS "RecastNavigation Gem uses ${RECAST_GIT_REPO} commit 5a870d4 (License: Zlib)")
    
set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")

# import each library as its static library from where it is located in the installer:
set(recastLibraries DebugUtils;Detour;DetourCrowd;DetourTileCache;Recast)
foreach(recastLibrary ${recastLibraries})
    add_library(3rdParty::RecastNavigation::${recastLibrary} STATIC IMPORTED GLOBAL)

    set_target_properties(3rdParty::RecastNavigation::${recastLibrary} 
        PROPERTIES 
            IMPORTED_LOCATION         "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}${recastLibrary}${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG   "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}${recastLibrary}${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELEASE "${BASE_LIBRARY_FOLDER}/release/${CMAKE_STATIC_LIBRARY_PREFIX}${recastLibrary}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    target_compile_definitions(3rdParty::RecastNavigation::${recastLibrary} INTERFACE DT_POLYREF64)
    ly_target_include_system_directories(TARGET 3rdParty::RecastNavigation::${recastLibrary} 
        INTERFACE 
            "${LY_ROOT_FOLDER}/Include/recastnavigation")
endforeach()

# notify O3DE That we have satisfied the RecastNavigation find_package requirements.
set(RecastNavigation_FOUND TRUE)

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::ogg)
    return()
endif()

# This file is used in the INSTALLER version of O3DE.  
# This file is included in cmake/3rdParty, which is already part of the search path for Findxxxxx.cmake files.

# It is still worth notifying people that they are accepting a 3rd Party Library here, and what license it uses, and
# where to get it.
set(ogg_GIT_REPO "https://github.com/xiph/ogg.git")
set(ogg_GIT_TAG "v1.3.6")
message(STATUS "MiniAudio Gem uses ${ogg_GIT_REPO} ${ogg_GIT_TAG} (BSD 3-Clause)")

set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")

add_library(ogg STATIC IMPORTED GLOBAL)
set_target_properties(ogg PROPERTIES 
    IMPORTED_LOCATION         "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}ogg${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_DEBUG   "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}ogg${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE "${BASE_LIBRARY_FOLDER}/release/${CMAKE_STATIC_LIBRARY_PREFIX}ogg${CMAKE_STATIC_LIBRARY_SUFFIX}")

ly_target_include_system_directories(TARGET ogg INTERFACE "${LY_ROOT_FOLDER}/include/ogg")

add_library(3rdParty::ogg ALIAS ogg)

# notify O3DE That we have satisfied the ogg find_package requirements.
set(ogg_FOUND TRUE)


#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# to use this script, invoke it using CMake script mode (-P option) 
# with the cwd being the engine root folder (the one with cmake as a subfolder)
# on the command line, define LY_3RDPARTY_PATH to a valid directory
# and PAL_PLATFORM_NAME to the platform you'd like to get or update python for.
# defines must come before the script call.
# example:
# cmake -DPAL_PLATFORM_NAME:string=Windows -DLY_3RDPARTY_PATH:string=%CMD_DIR% -P get_python.cmake

cmake_minimum_required(VERSION 3.22)

if(LY_3RDPARTY_PATH)
    file(TO_CMAKE_PATH ${LY_3RDPARTY_PATH} LY_3RDPARTY_PATH)
endif()

function(o3de_current_file_path path)
    set(${path} ${CMAKE_CURRENT_FUNCTION_LIST_DIR} PARENT_SCOPE)
endfunction()

o3de_current_file_path(current_path)
file(REAL_PATH ${current_path}/.. LY_ROOT_FOLDER)

set(LY_PACKAGE_KEEP_AFTER_DOWNLOADING FALSE)
include(cmake/LYPython.cmake)

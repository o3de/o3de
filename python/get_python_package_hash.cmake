#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# This script will get the current python package hash. To use this script, invoke it using CMake 
# script mode (-P option) with the cwd being the engine root folder (the one with cmake as a subfolder)
# on the command line, define LY_3RDPARTY_PATH to a valid directory
# and PAL_PLATFORM_NAME to the platform you'd like to get or update python for.
# defines must come before the script call.
# example:
# cmake -DPAL_PLATFORM_NAME:string=Windows -DLY_ROOT_FOLDER:string=%CMD_DIR% -P get_python_package_hash.cmake

cmake_minimum_required(VERSION 3.22)

if(${CMAKE_ARGC} LESS 4)
    message(FATAL_ERROR "Missing required engine root argument.")
endif()
if(${CMAKE_ARGC} LESS 5)
    message(FATAL_ERROR "Missing required platform name argument.")
endif()

#! ly_set: override the ly_set macro that the Python_<platform>.cmake file will use to set 
#          the environments. 
#
macro(ly_set name)
    set(${name} "${ARGN}")
    if(LY_PARENT_SCOPE)
        set(${name} "${ARGN}" PARENT_SCOPE)
    endif()
endmacro()

#! ly_associate_package: Stub out since this script is only reading the package hash
#
macro("ly_associate_package")
endmacro()

# The first required argument is the platform name
set(ENGINE_ROOT ${CMAKE_ARGV3})
set(PAL_PLATFORM_NAME ${CMAKE_ARGV4})

# The optional second argument is the architecture
if(${CMAKE_ARGC} GREATER 5)
    set(PLATFORM_ARCH "_${CMAKE_ARGV5}")
endif()

string(TOLOWER ${PAL_PLATFORM_NAME} PAL_PLATFORM_NAME_LOWERCASE)

include(${ENGINE_ROOT}/cmake/3rdParty/Platform/${PAL_PLATFORM_NAME}/Python_${PAL_PLATFORM_NAME_LOWERCASE}${PLATFORM_ARCH}.cmake)

# Note: using 'message(STATUS ..' will print to STDOUT, but will always include a double hyphen '--'. Instead we will 
# use the cmake echo command directly to do this
execute_process(COMMAND ${CMAKE_COMMAND} -E echo ${LY_PYTHON_PACKAGE_HASH})

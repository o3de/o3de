#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

# to use this script, invoke it using CMake script mode (-P option) 
# with the cwd being the engine root folder (the one with cmake as a subfolder)
# on the command line, define LY_3RDPARTY_PATH to a valid directory
# and PAL_PLATFORM_NAME to the platform you'd like to get or update python for.
# defines must come before the script call.
# example:
# cmake -DPAL_PLATFORM_NAME:string=Windows -DLY_3RDPARTY_PATH:string=%CMD_DIR% -P get_python.cmake

cmake_minimum_required(VERSION 3.17)

string(TOLOWER ${PAL_PLATFORM_NAME} PAL_PLATFORM_NAME_LOWERCASE)

include(cmake/3rdPartyPackages.cmake)

set(LY_PACKAGE_KEEP_AFTER_DOWNLOADING FALSE)
include(cmake/LYPython.cmake)

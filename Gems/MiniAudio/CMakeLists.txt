#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_gem_setup("MiniAudio")

# adds the 3rdParty folder into the CMAKE_MODULE_PATH.  This causes any call to find_package(XXXX)
# to look in this folder for `FindXXXX.cmake` before it looks elsewhere, and run that file if it can
# find it.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})

add_subdirectory(Code)

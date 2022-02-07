#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT ${CMAKE_ARGC} EQUAL 6)
    message(FATAL_ERROR "RPathChange script called with the wrong number of arguments")
endif()

file(RPATH_CHANGE FILE "${CMAKE_ARGV3}" OLD_RPATH "${CMAKE_ARGV4}" NEW_RPATH "${CMAKE_ARGV5}")
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

if(NOT ${CMAKE_ARGC} EQUAL 6)
    message(FATAL_ERROR "RPathChange script called with the wrong number of arguments")
endif()

file(RPATH_CHANGE FILE "${CMAKE_ARGV3}" OLD_RPATH "${CMAKE_ARGV4}" NEW_RPATH "${CMAKE_ARGV5}")
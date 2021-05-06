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

set(LY_WIX_PATH "" CACHE PATH "Path to the WiX install path")

if(LY_WIX_PATH)
    file(TO_CMAKE_PATH ${LY_QTIFW_PATH} CPACK_WIX_ROOT)
elseif(DEFINED ENV{WIX})
    file(TO_CMAKE_PATH $ENV{WIX} CPACK_WIX_ROOT)
endif()

if(CPACK_WIX_ROOT)
    if(NOT EXISTS ${CPACK_WIX_ROOT})
        message(FATAL_ERROR "Invalid path supplied for LY_WIX_PATH argument or WIX environment variable")
    endif()
else()
    # early out as no path to WiX has been supplied effectively disabling support
    return()
endif()

set(CPACK_GENERATOR "WIX")

set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/cmake/Platform/Windows/PackagingTemplate.wxs.in")

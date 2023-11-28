#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_AWSGAMELIFTSERVER_SUPPORTED TRUE)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.1.1-rev1-linux            TARGETS AWSGameLiftServerSDK        PACKAGE_HASH c503183bb1e5260336fc90dfcc784764c4b119e7de930b2ce4169f615342475e)

elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.1.1-rev1-linux-aarch64    TARGETS AWSGameLiftServerSDK        PACKAGE_HASH ed742f43285776b1a91e70bc3757f1190476fd484542c597364519651c2097aa)

else()

    message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
    
endif()


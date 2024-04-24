#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_AWSGAMELIFTSERVER_SUPPORTED TRUE)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.1.1-rev1-linux            TARGETS AWSGameLiftServerSDK        PACKAGE_HASH c652bd18c9100a3f7dcd4062d0cd01d0c094b62b2997731ed15edae1f7e691a3)

elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.1.1-rev1-linux-aarch64    TARGETS AWSGameLiftServerSDK        PACKAGE_HASH eb03413cb92fd64a8cd11abcf666a61ae62d4497d0dd2154e4ea2247aaf5aac7)

else()

    message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
    
endif()


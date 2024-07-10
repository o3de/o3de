#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_AWSGAMELIFTSERVER_SUPPORTED TRUE)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.1.2-rev1-linux            TARGETS AWSGameLiftServerSDK        PACKAGE_HASH 47324479faf2e42f9783462c53ac787273da6fc37912a795f47d8ac14d872a9e)

elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.1.2-rev1-linux-aarch64    TARGETS AWSGameLiftServerSDK        PACKAGE_HASH 16804a3975db609856e2d638d95913dade4a3066367082c78969adf2edfc5882)

else()

    message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
    
endif()


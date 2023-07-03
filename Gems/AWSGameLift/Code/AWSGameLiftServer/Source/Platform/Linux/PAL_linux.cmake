#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_AWSGAMELIFTSERVER_SUPPORTED TRUE)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.0.0-rev3-linux            TARGETS AWSGameLiftServerSDK        PACKAGE_HASH 2e51bbd06c67fe6f7dda99b8b59a742a0a6ca73151992be705de474d5253ed50)

elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.0.0-rev3-linux-aarch64    TARGETS AWSGameLiftServerSDK        PACKAGE_HASH 7513a501f074544e05bc49c66849648c1cb8ae544a6c397d94b1d967e9f362ac)

else()

    message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
    
endif()


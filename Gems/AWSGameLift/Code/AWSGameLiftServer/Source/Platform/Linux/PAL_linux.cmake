#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_AWSGAMELIFTSERVER_SUPPORTED TRUE)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.1.1-rev1-linux            TARGETS AWSGameLiftServerSDK        PACKAGE_HASH 2e51bbd06c67fe6f7dda99b8b59a742a0a6ca73151992be705de474d5253ed50)

elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.1.1-rev1-linux-aarch64    TARGETS AWSGameLiftServerSDK        PACKAGE_HASH b84791e792b7cd4a6dcb213fd756fd4bd4e3118425d85d8a618a474386e95bd3)

else()

    message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
    
endif()


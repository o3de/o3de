#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_AWSGAMELIFTSERVER_SUPPORTED TRUE)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.0.0-rev2-linux            TARGETS AWSGameLiftServerSDK        PACKAGE_HASH 08b6e897385d9ee6c97e411d875c3343d136815fa8835c27059be765384b89dc)

elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

    ly_associate_package(PACKAGE_NAME AWSGameLiftServerSDK-5.0.0-rev2-linux-aarch64    TARGETS AWSGameLiftServerSDK        PACKAGE_HASH f5b7806f2eacaa86f57126b6f807c6363d6fa289178d271c4b8b72981d613857)

else()

    message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
    
endif()


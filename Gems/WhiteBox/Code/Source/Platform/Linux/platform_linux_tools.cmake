#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-linux         TARGETS OpenMesh PACKAGE_HASH 805bd0b24911bb00c7f575b8c3f10d7ea16548a5014c40811894a9445f17a126)
    elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
        ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-linux-aarch64 TARGETS OpenMesh PACKAGE_HASH 0d53d215c4b2185879e3b27d1a4bdf61a53bcdb059eae30377ea4573bcd9ebc1)
    else()
        message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    set(LY_BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::OpenMesh)
endif()

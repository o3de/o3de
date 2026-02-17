#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev8-mac TARGETS PhysX4 PACKAGE_HASH 9c97e0af5acf0104a32bfeabe1685178617a3755d61d5910423a13b55ed40c54)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
    ly_associate_package(PACKAGE_NAME PhysX-4.1.2-rev9-mac-arm64 TARGETS PhysX4 PACKAGE_HASH 4cb1338ad9f65de5103777427641efded07530f626ea5bda88400fe833da31d4)
else()
    message(FATAL_ERROR "Unsupported mac architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(PAL_TRAIT_BUILD_HOST_TOOLS)
     if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
          ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-rev1-mac TARGETS poly2tri PACKAGE_HASH 23e49e6b06d79327985d17b40bff20ab202519c283a842378f5f1791c1bf8dbc) 
     elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
          ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-rev1-mac-arm64 TARGETS poly2tri PACKAGE_HASH 7a55c3fe80a75d19b78179e0a682ee1020bc66e1da6e482e8bb19f06f8b45c10)
     else()
          message(FATAL_ERROR "Unsupported mac architecture ${CMAKE_SYSTEM_PROCESSOR}")
     endif()
endif()

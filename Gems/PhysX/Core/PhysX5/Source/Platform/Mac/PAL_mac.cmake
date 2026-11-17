#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    ly_associate_package(PACKAGE_NAME PhysX-5.1.1-rev4-mac TARGETS PhysX5 PACKAGE_HASH f37c82c38fd309b2441cf0ac895ddd8fe883fcb43a894c7666982f2630066b3b)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
    ly_associate_package(PACKAGE_NAME PhysX-5.1.1-rev4-mac-arm64 TARGETS PhysX5 PACKAGE_HASH ac9cb2916df87a9d3f4d4ad85795bba0e03b3dadc70b8a817076356084594fa7)
else()
    message(FATAL_ERROR "Unsupported mac architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

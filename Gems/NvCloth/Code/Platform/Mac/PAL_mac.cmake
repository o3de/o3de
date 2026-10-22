#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    ly_associate_package(PACKAGE_NAME NvCloth-v1.1.6-4-gd243404-pr58-rev1-mac TARGETS NvCloth PACKAGE_HASH 9156548da6fc35f89b592306cc512257042159e3c9c2ce5f483d8d0701b63716)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
    ly_associate_package(PACKAGE_NAME NvCloth-v1.1.6-rev1-mac-arm64 TARGETS NvCloth PACKAGE_HASH 8e896bfc6a8bc1531aa595b5bcdb7e0ddb0c485730ba7c15943998d0727c8448)
else()
    message(FATAL_ERROR "Unsupported mac architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()


set(PAL_TRAIT_NVCLOTH_USE_STUB FALSE)

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    ly_associate_package(PACKAGE_NAME NvCloth-v1.1.6-4-gd243404-pr58-rev1-linux         TARGETS NvCloth PACKAGE_HASH c2e469c909fb358105ffe854476e87d9c69626aa16bd0ca0bcd941522adadc6d)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    ly_associate_package(PACKAGE_NAME NvCloth-v1.1.6-4-gd243404-pr58-rev1-linux-aarch64 TARGETS NvCloth PACKAGE_HASH 5cf6afce8a65f9584b25b99fc04843cd9e5e4720d2a1736321c7768d57409161)
else()
    message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(PAL_TRAIT_NVCLOTH_USE_STUB FALSE)

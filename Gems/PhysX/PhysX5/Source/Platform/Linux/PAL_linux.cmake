#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
   ly_associate_package(PACKAGE_NAME PhysX-5.1.1-rev4-linux TARGETS PhysX5 PACKAGE_HASH be2dee89394450f3227034c263c10599100decb9d1c24f5d1e5fab1945b2bfce)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    ly_associate_package(PACKAGE_NAME PhysX-5.1.1-rev4-linux-aarch64 TARGETS PhysX5 PACKAGE_HASH 0bf5dd4073f4d782117784b3fb2601601d5d34096d21fa46400746f0e9e50cbd)
else()
    message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-rev1-linux           TARGETS poly2tri PACKAGE_HASH b16eef8f0bc469de0e3056d28d7484cf42659667e39b68b239f0d3a4cbb533d0) 
        ly_associate_package(PACKAGE_NAME v-hacd-2.3-1a49edf-rev1-linux         TARGETS v-hacd   PACKAGE_HASH 777bed3c7805a63446ddfea51c505b35398744f9245fa7ddc58e6a25d034e682)
    elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
        ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-rev1-linux-aarch64   TARGETS poly2tri PACKAGE_HASH a7dfa76df0b77a65c00e52bda28c3c174fe84ca115f5a015b6e937ee5b55e969)
        ly_associate_package(PACKAGE_NAME v-hacd-2.3-1a49edf-rev1-linux-aarch64 TARGETS v-hacd   PACKAGE_HASH e5d45327099588f00f4bd0af3907f29279e23831a84c26661bb85248fb1d69c1)        
    else()
        message(FATAL_ERROR "Unsupported linux architecture ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

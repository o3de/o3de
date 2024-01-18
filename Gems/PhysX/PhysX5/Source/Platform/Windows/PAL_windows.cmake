#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

ly_associate_package(PACKAGE_NAME PhysX-5.1.1-rev3-windows TARGETS PhysX5 PACKAGE_HASH bdb335df8abe1e2a12609f5956ba2928d844f2194fbe5b3b8db04e1ee8106edb)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
     ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-rev1-windows TARGETS poly2tri PACKAGE_HASH 5fea2bf294e5130e0654fbfa39f192e6369f3853901dde90bb9b3f3a11edcb1e) 
     ly_associate_package(PACKAGE_NAME v-hacd-2.3-1a49edf-rev1-windows TARGETS v-hacd PACKAGE_HASH c5826ec28aedc3b5931ddf655d055395872cd3e75cd829f5745a2e607deb7468)
endif()

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

ly_associate_package(PACKAGE_NAME PhysX-4.1.2.29882248-rev8-windows TARGETS PhysX4 PACKAGE_HASH f2bf89213e32674fdf289d9ba7afadf2cdbaf112cafd22e8dc9de7a0f4499c84)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
     ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-rev1-windows TARGETS poly2tri PACKAGE_HASH 5fea2bf294e5130e0654fbfa39f192e6369f3853901dde90bb9b3f3a11edcb1e) 
     ly_associate_package(PACKAGE_NAME v-hacd-2.3-1a49edf-rev1-windows TARGETS v-hacd PACKAGE_HASH c5826ec28aedc3b5931ddf655d055395872cd3e75cd829f5745a2e607deb7468)
endif()

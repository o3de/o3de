#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

ly_associate_package(PACKAGE_NAME PhysX-5.6.1-rev1-windows TARGETS PhysX5 PACKAGE_HASH e379ea1d4dae491dfd05cc2b27d5894e32201ae7d497345d20936f4b7176fc4f)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
     ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-rev1-windows TARGETS poly2tri PACKAGE_HASH 5fea2bf294e5130e0654fbfa39f192e6369f3853901dde90bb9b3f3a11edcb1e)
endif()

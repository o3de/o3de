#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
     ly_associate_package(PACKAGE_NAME poly2tri-0.3.3-rev2-multiplatform TARGETS poly2tri PACKAGE_HASH 04092d06716f59b936b61906eaf3647db23b685d81d8b66131eb53e0aeaa1a38) 
     ly_associate_package(PACKAGE_NAME v-hacd-2.0-rev1-multiplatform TARGETS v-hacd PACKAGE_HASH 5c71aef19cc9787d018d64eec076e9f51ea5a3e0dc6b6e22e57c898f6cc4afe3)
endif()

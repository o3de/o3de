#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

ly_associate_package(PACKAGE_NAME PhysX-5.1.1-rev4-mac TARGETS PhysX5 PACKAGE_HASH f37c82c38fd309b2441cf0ac895ddd8fe883fcb43a894c7666982f2630066b3b)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
     ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-rev1-mac TARGETS poly2tri PACKAGE_HASH 23e49e6b06d79327985d17b40bff20ab202519c283a842378f5f1791c1bf8dbc) 
     ly_associate_package(PACKAGE_NAME v-hacd-2.3-1a49edf-rev1-mac TARGETS v-hacd PACKAGE_HASH c1de9fadb2c0db42416a1bd5fe3423401c6f41b58409bc652064b5ad23667c09)
endif()

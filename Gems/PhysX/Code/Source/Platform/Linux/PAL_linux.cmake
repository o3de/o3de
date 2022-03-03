#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
     ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-rev1-linux TARGETS poly2tri PACKAGE_HASH b16eef8f0bc469de0e3056d28d7484cf42659667e39b68b239f0d3a4cbb533d0) 
     ly_associate_package(PACKAGE_NAME v-hacd-2.3-1a49edf-rev1-linux TARGETS v-hacd PACKAGE_HASH 777bed3c7805a63446ddfea51c505b35398744f9245fa7ddc58e6a25d034e682)
endif()

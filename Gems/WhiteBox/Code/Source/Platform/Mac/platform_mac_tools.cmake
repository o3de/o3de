#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-mac TARGETS OpenMesh PACKAGE_HASH af92db02a25c1f7e1741ec898f49d81d52631e00336bf9bddd1e191590063c2f)

    set(LY_BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::OpenMesh)
endif()

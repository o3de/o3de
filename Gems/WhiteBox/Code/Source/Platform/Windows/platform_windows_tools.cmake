#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev1-windows TARGETS OpenMesh PACKAGE_HASH 1c1df639358526c368e790dfce40c45cbdfcfb1c9a041b9d7054a8949d88ee77)

    set(LY_BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::OpenMesh)
endif()

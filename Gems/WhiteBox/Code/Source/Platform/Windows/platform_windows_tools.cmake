#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_associate_package(PACKAGE_NAME OpenMesh-8.1-rev3-windows TARGETS OpenMesh PACKAGE_HASH 7a6309323ad03bfc646bd04ecc79c3711de6790e4ff5a72f83a8f5a8f496d684)

    set(LY_BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::OpenMesh)
endif()

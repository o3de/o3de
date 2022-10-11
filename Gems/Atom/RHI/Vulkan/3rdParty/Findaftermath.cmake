#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(EXISTS "${ATOM_AFTERMATH_PATH_CMAKE_FORMATTED}/include/GFSDK_Aftermath.h")
    ly_add_external_target(
        NAME Aftermath
        VERSION
        3RDPARTY_ROOT_DIRECTORY ${ATOM_AFTERMATH_PATH_CMAKE_FORMATTED}
        INCLUDE_DIRECTORIES include
    )
endif()



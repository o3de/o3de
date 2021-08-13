#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(LY_PIX_ENABLED)
    file(TO_CMAKE_PATH "${LY_PIX_PATH}" PIX_PATH)
    message(STATUS "PIX PATH ${PIX_PATH}")

    ly_add_external_target(
        NAME pix
        3RDPARTY_ROOT_DIRECTORY "${PIX_PATH}"
        VERSION
        INCLUDE_DIRECTORIES include
    )
endif()

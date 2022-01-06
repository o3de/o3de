#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_PIX_PATH "${LY_3RDPARTY_PATH}/winpixeventruntime" CACHE PATH "Path to the Windows Pix Event Runtime.")

ly_add_external_target(
    NAME pix
    3RDPARTY_ROOT_DIRECTORY ${LY_PIX_PATH}
    VERSION
    INCLUDE_DIRECTORIES Include
    COMPILE_DEFINITIONS USE_PIX
)


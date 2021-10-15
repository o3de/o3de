#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_external_target(
    NAME renderdoc
    3RDPARTY_ROOT_DIRECTORY "${LY_RENDERDOC_PATH}"
    VERSION
    COMPILE_DEFINITIONS USE_RENDERDOC
)

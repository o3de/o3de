#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_external_target(
    NAME RadTelemetry
    3RDPARTY_ROOT_DIRECTORY "${LY_RAD_TELEMETRY_INSTALL_ROOT}"
    VERSION 3.5.0.17
    INCLUDE_DIRECTORIES Include
)

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# Ly Remote Console tests.
#

# Unit tests.
ly_add_pytest(
    NAME RemoteConsole_UnitTests_main_no_gpu
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit/
    LABELS REQUIRES_tiaf
)
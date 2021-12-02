#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Platform specific cmake file for configuring target compiler/link properties
# based on the active platform
# NOTE: functions in cmake are global, therefore adding functions to this file
# is being avoided to prevent overriding functions declared in other targets platfrom
# specific cmake files

set(LY_BUILD_DEPENDENCIES
    PRIVATE
        dinput8
        dxguid
)

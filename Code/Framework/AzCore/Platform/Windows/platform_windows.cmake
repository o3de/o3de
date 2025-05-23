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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    SET(LY_LINK_OPTIONS
        PRIVATE
            /IGNORE:4217        # Ignore AzCore static including buses from AzCore shared (circular)
    )
endif()
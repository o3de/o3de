#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(cmake/Platform/Common/Configurations_common.cmake)

ly_append_configurations_options(
    COMPILATION
        -fno-exceptions
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -Wall
        -Werror

        # Disabled warnings (please do not disable any others without first consulting ly-warnings)
        -Wrange-loop-analysis
        -Wno-unknown-warning-option # used as a way to mark warnings that are MSVC only
        -Wno-format-security
        -Wno-inconsistent-missing-override
        -Wno-parentheses
        -Wno-reorder
        -Wno-switch
        -Wno-undefined-var-template

    COMPILATION_DEBUG
        -O0 # No optimization
        -g # debug symbols
        -fno-inline # don't inline functions
        -fstack-protector # Add additional checks to catch stack corruption issues
    COMPILATION_PROFILE
        -O2
        -g # debug symbols
    COMPILATION_RELEASE
        -O2
    LINK_NON_STATIC
        -Wl,-undefined,error
)

include(cmake/Platform/Common/TargetIncludeSystemDirectories_supported.cmake)


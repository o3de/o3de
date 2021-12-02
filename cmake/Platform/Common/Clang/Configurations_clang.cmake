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

        ###################
        # Disabled warnings (please do not disable any others without first consulting sig-build)
        ###################
        -Wno-inconsistent-missing-override # unfortunately there is no warning in MSVC to detect missing overrides, 
            # MSVC's static analyzer can, but that is a different run that most developers are not aware of. A pass
            # was done to fix all hits. Leaving this disabled until there is a matching warning in MSVC.

        -Wrange-loop-analysis
        -Wno-unknown-warning-option # used as a way to mark warnings that are MSVC only
        -Wno-parentheses
        -Wno-reorder
        -Wno-switch
        -Wno-undefined-var-template
        
        ###################
        # Enabled warnings (that are disabled by default)
        ###################

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


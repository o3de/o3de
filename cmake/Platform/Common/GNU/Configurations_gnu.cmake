#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

include(cmake/Platform/Common/Configurations_common.cmake)

ly_append_configurations_options(
    COMPILATION
        -fno-exceptions
        -fvisibility=hidden
        -Wall

        # Disabled warnings (please do not disable any others without first consulting ly-warnings)
        -Wno-format-security
        -Wno-multichar
        -Wno-parentheses
        -Wno-switch
        -Wno-tautological-compare
        -Wno-unknown-pragmas
        -Wno-unused-function
        -Wno-unused-value
        -Wno-unused-variable
        -Wno-format-truncation
        -Wno-reorder
        -Wno-uninitialized
        -Wno-array-bounds

    COMPILATION_C
        -Wno-absolute-value
        -Werror

    COMPILATION_CXX
        -fvisibility-inlines-hidden
        -Wno-invalid-offsetof
        -Werror

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
)

include(cmake/Platform/Common/TargetIncludeSystemDirectories_supported.cmake)


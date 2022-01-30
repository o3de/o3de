#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(cmake/Platform/Common/Configurations_common.cmake)

set(LY_GCC_BUILD_FOR_GCOV FALSE CACHE BOOL "Flag to enable the build for gcov usage")
if(LY_GCC_BUILD_FOR_GCOV)
    set(LY_GCC_GCOV_FLAGS "--coverage")
endif()

set(LY_GCC_BUILD_FOR_GPROF FALSE CACHE BOOL "Flag to enable the build for gprof usage")
if(LY_GCC_BUILD_FOR_GPROF)
    set(LY_GCC_GPROF_FLAGS "-pg")
endif()


ly_append_configurations_options(

    COMPILATION_C
        -fno-exceptions
        -fvisibility=hidden
        -Wall
        -Werror

        ${LY_GCC_GCOV_FLAGS}
        ${LY_GCC_GPROF_FLAGS}

    COMPILATION_CXX
        -fno-exceptions
        -fvisibility=hidden
        -fvisibility-inlines-hidden

        -Wall
        -Werror

        ${LY_GCC_GCOV_FLAGS}
        ${LY_GCC_GPROF_FLAGS}

        # Disabled warnings 
        -Wno-array-bounds
        -Wno-attributes
        -Wno-class-memaccess
        -Wno-comment
        -Wno-delete-non-virtual-dtor
        -Wno-enum-compare
        -Wno-format-overflow
        -Wno-format-truncation
        -Wno-int-in-bool-context
        -Wno-logical-not-parentheses
        -Wno-memset-elt-size
        -Wno-nonnull-compare
        -Wno-parentheses
        -Wno-reorder
        -Wno-restrict
        -Wno-return-local-addr
        -Wno-sequence-point
        -Wno-sign-compare
        -Wno-strict-aliasing
        -Wno-stringop-overflow
        -Wno-stringop-truncation
        -Wno-switch
        -Wno-uninitialized
        -Wno-unused-but-set-variable
        -Wno-unused-result
        -Wno-unused-value
        -Wno-unused-variable

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

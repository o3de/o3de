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
    COMPILATION
        -fno-exceptions
        -fvisibility=hidden
        -Wall

        ${LY_GCC_GCOV_FLAGS}
        ${LY_GCC_GPROF_FLAGS}

        # Disabled warnings 
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
        -Wno-class-memaccess
        -Wno-nonnull-compare
        -Wno-strict-aliasing
        -Wno-unused-result
        -Wno-sign-compare
        -Wno-return-local-addr
        -Wno-stringop-overflow
        -Wno-attributes
        -Wno-logical-not-parentheses
        -Wno-stringop-truncation
        -Wno-memset-elt-size
        -Wno-unused-but-set-variable
        -Wno-enum-compare
        -Wno-int-in-bool-context
        -Wno-sequence-point
        -Wno-delete-non-virtual-dtor

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


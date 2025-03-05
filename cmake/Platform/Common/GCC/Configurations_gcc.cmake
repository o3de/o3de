#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(cmake/Platform/Common/Configurations_common.cmake)

# Exceptions are disabled by default.  Use this to turn them on just for a specific target.
set(O3DE_COMPILE_OPTION_ENABLE_EXCEPTIONS PUBLIC -fexceptions)

# O3DE Sets visibility to hidden by default, requiring explicit export.
# However, some 3rd Party libraries will not export any symbols with this setting
# as they expect to be built with visibility set to default.  Use this compile option
# to turn visibility back to default for those 3rd Party targets ONLY.
set(O3DE_COMPILE_OPTION_EXPORT_SYMBOLS PRIVATE -fvisibility=default)

# By default, O3DE sets warning level 4 and sets warnings as errors.  If you're pulling in
# external code (from 3rd Party libraries) you can't really control whether they generate
# warnings or not, and its usually out of scope to fix them.  Add this compile option to 
# those 3rd Party targets ONLY.
set(O3DE_COMPILE_OPTION_DISABLE_WARNINGS PRIVATE -w)

set(LY_GCC_BUILD_FOR_GCOV FALSE CACHE BOOL "Flag to enable the build for gcov usage")
if(LY_GCC_BUILD_FOR_GCOV)
    set(LY_GCC_GCOV_FLAGS "--coverage")
endif()

set(LY_GCC_BUILD_FOR_GPROF FALSE CACHE BOOL "Flag to enable the build for gprof usage")
if(LY_GCC_BUILD_FOR_GPROF)
    set(LY_GCC_GPROF_FLAGS "-pg")
endif()


ly_append_configurations_options(
    DEFINES_PROFILE
        _FORTIFY_SOURCE=2
    DEFINES_RELEASE
        _FORTIFY_SOURCE=2

    COMPILATION_C
        -fno-exceptions
        -fvisibility=hidden
        -Wall
        -Werror

        -fpie                   # Position-Independent Executables
        -fstack-protector-all   # Enable stack protectors for all functions

        ${LY_GCC_GCOV_FLAGS}
        ${LY_GCC_GPROF_FLAGS}

    COMPILATION_CXX
        -fno-exceptions
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -Wall
        -Werror

        -fpie                   # Position-Independent Executables
        -fstack-protector-all   # Enable stack protectors for all functions

        ${LY_GCC_GCOV_FLAGS}
        ${LY_GCC_GPROF_FLAGS}

        # Disabled warnings
        -Wno-array-bounds
        -Wno-attributes
        -Wno-class-memaccess
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
        -Wno-sequence-point
        -Wno-sign-compare
        -Wno-strict-aliasing
        -Wno-stringop-overflow
        -Wno-stringop-truncation
        -Wno-switch
        -Wno-uninitialized
        -Wno-unused-result

    COMPILATION_DEBUG
        -O0 # No optimization
        -g # debug symbols
        -fno-inline # don't inline functions
    COMPILATION_PROFILE
        -O2
        -g # debug symbols
    COMPILATION_RELEASE
        -O2

    LINK_NON_STATIC
        -Wl,-undefined,error
        -fpie
        -Wl,-z,relro,-z,now
        -Wl,-z,noexecstack
    LINK_EXE
        -pie
        -fpie
        -Wl,-z,relro,-z,now
        -Wl,-z,noexecstack

)

include(cmake/Platform/Common/TargetIncludeSystemDirectories_supported.cmake)

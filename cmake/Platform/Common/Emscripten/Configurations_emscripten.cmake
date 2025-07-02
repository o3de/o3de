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

ly_append_configurations_options(
    DEFINES_PROFILE
        _FORTIFY_SOURCE=2
    DEFINES_RELEASE
        _FORTIFY_SOURCE=2
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
        -Wno-deprecated-no-relaxed-template-template-args

        ###################
        # Enabled warnings (that are disabled by default)
        ###################

    COMPILATION_DEBUG
        -O0                         # No optimization
        -g                          # debug symbols
        -fno-inline                 # don't inline functions

        -fstack-protector-all       # Enable stack protectors for all functions
        -fstack-check

    COMPILATION_PROFILE
        -O2
        -g                          # debug symbols

        -fstack-protector-all       # Enable stack protectors for all functions
        -fstack-check

    COMPILATION_RELEASE
        -O2
)

if(LY_BUILD_WITH_ADDRESS_SANITIZER)
    ly_append_configurations_options(
        COMPILATION_DEBUG
            -fsanitize=address
            -fno-omit-frame-pointer
        LINK_NON_STATIC_DEBUG
            -shared-libsan
            -fsanitize=address
    )
endif()
include(cmake/Platform/Common/TargetIncludeSystemDirectories_supported.cmake)


#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

ly_add_3rdparty_target(
    NAME mcpp STATIC
    INCLUDE_DIRECTORIES
        PUBLIC Include
        PRIVATE Source
    COMPILE_DEFINITIONS
        PRIVATE
            MCPP_LIB=1
            $<$<NOT:$<PLATFORM_ID:Windows>>:HAVE_CONFIG_H=1>
    TARGET_PROPERTIES
        C_STANDARD 90
        C_STANDARD_REQUIRED YES
)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(mcpp PRIVATE -Wno-implicit-function-declaration)
endif()

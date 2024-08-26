#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Set compiler options specific to this compiler
message("Gene COMPILE_OPTIONS: ${COMPILE_OPTIONS}")
 set_property(
    DIRECTORY
    APPEND_STRING
    PROPERTY COMPILE_OPTIONS "/EHsc /wd4777"
)

#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_source_properties(
    SOURCES
        GridMate/Carrier/SecureSocketDriver.cpp
        GridMate/Carrier/StreamSecureSocketDriver.cpp
    PROPERTY COMPILE_OPTIONS
    VALUES -Wno-deprecated-declarations
)

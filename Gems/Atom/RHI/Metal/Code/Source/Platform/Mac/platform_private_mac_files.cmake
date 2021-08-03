#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Metal_Traits_Platform.h
    Metal_Traits_Mac.h
    RHI/Metal_RHI_Mac.cpp
    RHI/MetalView_Mac.mm
    RHI/MetalView_Mac.h
    RHI/MetalView_Platform.h
    RHI/Conversions_Platform.h
    RHI/Conversions_Mac.cpp
    RHI/Conversions_Mac.h
)

ly_add_source_properties(
    SOURCES Source/Platform/Mac/RHI/MetalView_Mac.mm
    PROPERTY COMPILE_OPTIONS
    VALUES -xobjective-c++
)

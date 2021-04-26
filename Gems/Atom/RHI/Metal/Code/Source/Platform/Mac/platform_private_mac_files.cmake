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

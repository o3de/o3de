#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# win2012 nodes can build DX11/DX12, but can't run. Disable runtime support if d3d12.dll/d3d11.dll is not available
set(PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED FALSE)
find_file(d3d12_dll
    d3d12.dll
    PATHS
        "$ENV{SystemRoot}\\System32"
)
mark_as_advanced(d3d12_dll)
if(d3d12_dll)
    set(PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED TRUE)
endif()

unset(pix3_header CACHE)

set(PAL_TRAIT_AFTERMATH_AVAILABLE FALSE)
unset(aftermath_header CACHE)
file(TO_CMAKE_PATH "$ENV{ATOM_AFTERMATH_PATH}" ATOM_AFTERMATH_PATH_CMAKE_FORMATTED)
find_file(aftermath_header
    GFSDK_Aftermath.h
    PATHS
        "${ATOM_AFTERMATH_PATH_CMAKE_FORMATTED}/include"
)  
mark_as_advanced(CLEAR, aftermath_header)
if(aftermath_header)
    set(PAL_TRAIT_AFTERMATH_AVAILABLE TRUE)
endif()

ly_add_source_properties(
    SOURCES
        Source/RHI.Builders/ShaderPlatformInterfaceSystemComponent.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES ${LY_PAL_TOOLS_DEFINES}
)

# Disable windows OS version check until infra can upgrade all our jenkins nodes
# if(NOT CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "10.0.17763")
#   message(FATAL_ERROR "Windows DX12 RHI implementation requires an OS version and SDK matching windows 10 build 1809 or greater")
# endif()

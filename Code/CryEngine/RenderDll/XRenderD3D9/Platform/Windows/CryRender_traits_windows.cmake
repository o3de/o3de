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

find_file(d3d12_header
    d3d12.h
    PATHS
        "$ENV{ProgramFiles\(x86\)}\\Windows Kits\\10\\Include\\${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\\um"
)
mark_as_advanced(d3d12_header)
if(NOT d3d12_header)
    message(FATAL_ERROR "Direct3D 12 header was not found in $ENV{ProgramFiles\(x86\)}\\Windows Kits\\10\\Include\\${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\\um")
endif()

set(CRYRENDER_TRAIT_BUILD_DX11_SUPPORTED TRUE)
set(CRYRENDER_TRAIT_BUILD_DX12_SUPPORTED TRUE)

# win2012 nodes can build DX11/DX12, but can't run. Disable runtime support if d3d12.dll/d3d11.dll is not available
set(CRYRENDER_TRAIT_RUNTIME_DX12_SUPPORTED FALSE)
find_file(d3d12_dll
    d3d12.dll
    PATHS
        "$ENV{SystemRoot}\\System32"
)
mark_as_advanced(d3d12_dll)
if(d3d12_dll)
    set(CRYRENDER_TRAIT_RUNTIME_DX12_SUPPORTED TRUE)
endif()

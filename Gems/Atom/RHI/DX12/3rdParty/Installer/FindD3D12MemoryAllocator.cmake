#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::D3D12MemoryAllocator)
    return()
endif()

set(DX12MA_GIT_REPO "https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git")
set(DX12MA_GIT_TAG "0.11.22")
message(STATUS "D3D12MemoryAllocator Gem uses ${DX12MA_GIT_REPO} ${DX12MA_GIT_TAG} (MIT)")

set(BASE_LIBRARY_FOLDER "${LY_ROOT_FOLDER}/lib/${PAL_PLATFORM_NAME}")

add_library(D3D12MemoryAllocator STATIC IMPORTED GLOBAL)
set_target_properties(D3D12MemoryAllocator PROPERTIES 
    IMPORTED_LOCATION         "${BASE_LIBRARY_FOLDER}/profile/${CMAKE_STATIC_LIBRARY_PREFIX}D3D12MA${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_DEBUG   "${BASE_LIBRARY_FOLDER}/debug/${CMAKE_STATIC_LIBRARY_PREFIX}D3D12MA${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE "${BASE_LIBRARY_FOLDER}/release/${CMAKE_STATIC_LIBRARY_PREFIX}D3D12MA${CMAKE_STATIC_LIBRARY_SUFFIX}")
ly_target_include_system_directories(TARGET D3D12MemoryAllocator INTERFACE "${LY_ROOT_FOLDER}/include/D3D12MemoryAllocator")
add_library(3rdParty::D3D12MemoryAllocator ALIAS D3D12MemoryAllocator)

set(D3D12MemoryAllocator_FOUND TRUE)

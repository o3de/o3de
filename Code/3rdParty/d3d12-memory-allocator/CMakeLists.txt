#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(d3d12-memory-allocator
    VERSION "3.3.0"
    LICENSE "MIT"
    URL "https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator/archive/refs/tags/v3.3.0.tar.gz"
    URL_HASH "04e3ef68cf2608c14c74a0b0190fb3c5e3cbb62e22fb8282ea62a9411d054133"
    GIT "https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git"
    GIT_HASH "600949266be004553f74c9ef809384380a9d44cf"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(d3d12-memory-allocator)

file(
    COPY "${d3d12-memory-allocator_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

o3de_disable_warnings()

ly_add_target(
    NAME d3d12-memory-allocator STATIC
    NAMESPACE 3rdParty
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/Include"
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::directx-headers
        PRIVATE
            d3d12
            dxgi
            dxguid
    COMPILE_DEFINITIONS
        PUBLIC D3D12MA_USING_DIRECTX_HEADERS
)

set_target_properties(d3d12-memory-allocator PROPERTIES SYSTEM TRUE)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/d3d12-memory-allocator
)

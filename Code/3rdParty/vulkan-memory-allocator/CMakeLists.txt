#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_fetch_content(vulkan-memory-allocator
    VERSION "3.4.0"
    LICENSE "MIT"
    URL "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.4.0.tar.gz"
    URL_HASH "822aa850c6ce77346ae96a8a1d351d52e77e85929f35363849a0a4e638e0a2a1"
    GIT "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
    GIT_HASH "3aa921224c154a0d2c43912bc88e1c42ce1f7607"
    NO_ADD_SUBDIR
)
FetchContent_MakeAvailable(vulkan-memory-allocator)

file(
    COPY "${vulkan-memory-allocator_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Include"
)

ly_add_target(
    NAME vulkan-memory-allocator HEADERONLY
    NAMESPACE 3rdParty
    FILES_CMAKE files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/Include"
    COMPILE_DEFINITIONS
        INTERFACE
            VMA_DYNAMIC_VULKAN_FUNCTIONS=1
            VMA_STATIC_VULKAN_FUNCTIONS=0
)

set_target_properties(vulkan-memory-allocator PROPERTIES SYSTEM TRUE)

ly_install_files(
    FILES
        LICENSE.txt
        PackageInfo.json
    DESTINATION Code/3rdParty/vulkan-memory-allocator
)

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::amd_vma)
    return()
endif()

block()
    o3de_fetch_content(VulkanMemoryAllocator
        VERSION "v3.4.0"
        LICENSE "MIT"
        URL "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.4.0.tar.gz"
        URL_HASH "822aa850c6ce77346ae96a8a1d351d52e77e85929f35363849a0a4e638e0a2a1"
        GIT "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
        GIT_HASH "3aa921224c154a0d2c43912bc88e1c42ce1f7607"
        PATCH_FILES "${CMAKE_CURRENT_LIST_DIR}/vma-disable-warning.patch"
        EXCLUDE_FROM_ALL
    )

    set(VMA_ENABLE_INSTALL OFF)

    # the below line is what actualy runs its CMakeList.txt file and executes targets and so on:
    FetchContent_MakeAvailable(VulkanMemoryAllocator)

    target_compile_definitions(VulkanMemoryAllocator INTERFACE VMA_STATIC_VULKAN_FUNCTIONS=0 VMA_DYNAMIC_VULKAN_FUNCTIONS=0)

    add_library(3rdParty::amd_vma ALIAS VulkanMemoryAllocator)
    ly_install(FILES ${CMAKE_CURRENT_LIST_DIR}/Installer/Findamd_vma.cmake DESTINATION cmake/3rdParty)
endblock()
set(amd_vma_FOUND TRUE PARENT_SCOPE)

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

set(AMD_VMA_GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git")
set(AMD_VMA_VERSION_STRING "v3.4.0")

message(STATUS "AzCore uses amd_vma ${AMD_VMA_VERSION_STRING} (MIT) from ${AMD_VMA_GIT_REPOSITORY}")

add_library(amd_vma IMPORTED INTERFACE GLOBAL)
add_library(3rdParty::amd_vma ALIAS amd_vma)

set(amd_vma_FOUND TRUE)

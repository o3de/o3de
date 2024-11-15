#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

get_property(atom_rhi_vulkan_gem_root GLOBAL PROPERTY "@GEMROOT:Atom_RHI_Vulkan@")
ly_add_external_target(
    NAME amd_vma
    VERSION 3.1.0-009ecd1
    3RDPARTY_ROOT_DIRECTORY ${atom_rhi_vulkan_gem_root}/External/vma
    INCLUDE_DIRECTORIES include
    COMPILE_DEFINITIONS VMA_STATIC_VULKAN_FUNCTIONS=0 VMA_DYNAMIC_VULKAN_FUNCTIONS=0
)

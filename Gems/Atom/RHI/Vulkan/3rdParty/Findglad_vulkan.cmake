#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

get_property(atom_rhi_vulkan_gem_root GLOBAL PROPERTY "@GEMROOT:Atom_RHI_Vulkan@")
ly_add_external_target(
    NAME glad_vulkan
    VERSION 2.0.0-beta
    3RDPARTY_ROOT_DIRECTORY ${atom_rhi_vulkan_gem_root}/External/glad
    INCLUDE_DIRECTORIES include
)

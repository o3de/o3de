/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define AZ_TRAIT_ATOM_SHADERBUILDER_DXC "Builders/DirectXShaderCompiler/bin/dxc"
#define AZ_TRAIT_ATOM_AZSL_SHADER_HEADER "" // Mac can not build vulkan shaders at the moment
#define AZ_TRAIT_ATOM_AZSL_PLATFORM_HEADER "" // Mac can not build vulkan shaders at the moment
#define AZ_TRAIT_ATOM_MOBILE_AZSL_SHADER_HEADER ""
#define AZ_TRAIT_ATOM_MOBILE_AZSL_PLATFORM_HEADER ""
#define AZ_TRAIT_ATOM_VULKAN_DISABLE_DUAL_SOURCE_BLENDING 0
#define AZ_TRAIT_ATOM_VULKAN_RECREATE_SWAPCHAIN_WHEN_SUBOPTIMAL 1
#define AZ_TRAIT_ATOM_VULKAN_SWAPCHAIN_SCALING_FLAGS AZ::RHI::ScalingFlags::None

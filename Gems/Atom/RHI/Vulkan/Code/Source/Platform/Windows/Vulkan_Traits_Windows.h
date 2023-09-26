/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define AZ_TRAIT_ATOM_SHADERBUILDER_DXC "Builders/DirectXShaderCompiler/dxc.exe"
#define AZ_TRAIT_ATOM_AZSL_SHADER_HEADER "Builders/ShaderHeaders/Platform/Windows/Vulkan/AzslcHeader.azsli"
#define AZ_TRAIT_ATOM_AZSL_PLATFORM_HEADER "Builders/ShaderHeaders/Platform/Windows/Vulkan/PlatformHeader.hlsli"
#define AZ_TRAIT_ATOM_MOBILE_AZSL_SHADER_HEADER "Builders/ShaderHeaders/Platform/Android/Vulkan/AzslcHeader.azsli"
#define AZ_TRAIT_ATOM_MOBILE_AZSL_PLATFORM_HEADER "Builders/ShaderHeaders/Platform/Android/Vulkan/PlatformHeader.hlsli"
#define AZ_TRAIT_ATOM_VULKAN_DISABLE_DUAL_SOURCE_BLENDING 0
#define AZ_TRAIT_ATOM_VULKAN_RECREATE_SWAPCHAIN_WHEN_SUBOPTIMAL 1
#define AZ_TRAIT_ATOM_VULKAN_SWAPCHAIN_SCALING_FLAGS AZ::RHI::ScalingFlags::None

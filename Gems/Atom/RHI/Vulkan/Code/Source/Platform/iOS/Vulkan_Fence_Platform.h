/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Device.h>
#include <vulkan/vulkan.h>

namespace AZ::Vulkan
{
#define AZ_VULKAN_CROSS_DEVICE_SEMAPHORES_SUPPORTED 0

    static constexpr const char* ExternalSemaphoreExtensionName = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
} // namespace AZ::Vulkan

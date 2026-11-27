/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <OpenXRVk_Platform.h>

namespace OpenXRVk::PhysicalDevice
{
    //! API to enumerate and return physical devices.
    AZStd::vector<VkPhysicalDevice> EnumerateDeviceList(XrSystemId xrSystemId, XrInstance xrInstance, VkInstance vkInstance); 
}


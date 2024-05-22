/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/base.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <vulkan/vulkan.h>

namespace AZ
{
    namespace Vulkan
    {
        //! A specific allocator to manage small objects used by Vulkan.
        //! Take over the allocations of small objects and expect to improve efficiency.
        class VkSystemAllocator 
        {
        public:
            static VkAllocationCallbacks* Get();
        private:
            VkSystemAllocator();
            ~VkSystemAllocator();

            AZStd::unique_ptr<VkAllocationCallbacks> m_allocationCallbacks;
        };
    }
}

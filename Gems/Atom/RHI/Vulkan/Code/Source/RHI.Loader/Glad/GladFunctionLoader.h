/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <Atom/RHI.Loader/FunctionLoader.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        class GladFunctionLoader:
            public FunctionLoader
        {
        public:
            AZ_CLASS_ALLOCATOR(GladFunctionLoader, AZ::SystemAllocator, 0);

            GladFunctionLoader() = default;
            virtual ~GladFunctionLoader() = default;

        private:
            //////////////////////////////////////////////////////////////////////////
            // FunctionLoader
            bool InitInternal() override;
            void ShutdownInternal() override;
            bool LoadProcAddresses(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) override;

            VkDevice m_device = VK_NULL_HANDLE;
        };
    } // namespace Vulkan
} // namespace AZ

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

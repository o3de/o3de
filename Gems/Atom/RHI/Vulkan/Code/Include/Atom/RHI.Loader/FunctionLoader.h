/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <Atom/RHI/Object.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class DynamicModuleHandle;

    namespace Vulkan
    {
        class FunctionLoader
        {
        public:
            AZ_CLASS_ALLOCATOR(FunctionLoader, AZ::SystemAllocator, 0);

            static AZStd::unique_ptr<FunctionLoader> Create();

            bool Init();
            void Shutdown();

            virtual ~FunctionLoader();

            virtual bool LoadProcAddresses(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) = 0;

        protected:

            virtual bool InitInternal() = 0;
            virtual void ShutdownInternal() = 0;

            AZStd::unique_ptr<AZ::DynamicModuleHandle> m_moduleHandle;
        };
    } // namespace Vulkan
} // namespace AZ

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

#include <Atom/RHI/DeviceObject.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/SignalEvent.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        class Semaphore final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(Semaphore, AZ::ThreadPoolAllocator, 0);

            using WaitSemaphore = AZStd::pair<VkPipelineStageFlags, RHI::Ptr<Semaphore>>;

            static RHI::Ptr<Semaphore> Create();
            RHI::ResultCode Init(Device& device);
            ~Semaphore() = default;

            void SignalEvent();
            void WaitEvent() const;
            void ResetSignalEvent();
            void SetRecycleValue(bool canRecycle);
            bool GetRecycleValue() const;
            VkSemaphore GetNativeSemaphore() const;

        private:
            Semaphore() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            VkSemaphore m_nativeSemaphore = VK_NULL_HANDLE;
            AZ::Vulkan::SignalEvent m_signalEvent;
            bool m_recyclable = true;
        };
    }
}

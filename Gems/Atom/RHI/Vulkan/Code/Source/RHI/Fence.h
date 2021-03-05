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

#include <Atom/RHI/Fence.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/SignalEvent.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        class Fence final
            : public RHI::Fence
        {
            using Base = RHI::Fence;

        public:
            AZ_CLASS_ALLOCATOR(Fence, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Fence, "AAAD0A37-5F85-4A68-9464-06EDAC6D62B0", Base);

            static RHI::Ptr<Fence> Create();
            ~Fence() = default;

            // According to the Vulkan Standard, fences can only be waited after the event
            // that will signal it is submitted. Because of this we need to tell the fence that it's
            // okay to start waiting on the native fence.
            void SignalEvent();
            VkFence GetNativeFence() const;

        private:
            Fence() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////
            // RHI::Fence
            RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState) override;
            void ShutdownInternal() override;
            void SignalOnCpuInternal() override;
            void WaitOnCpuInternal() const override;
            void ResetInternal() override;
            RHI::FenceState GetFenceStateInternal() const override;
            //////////////////////////////////////////////////////////////////////

            VkFence m_nativeFence = VK_NULL_HANDLE;

            AZ::Vulkan::SignalEvent m_signalEvent;
        };
    }
}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<Fence> Fence::Create()
        {
            return aznew Fence();
        }

        void Fence::SignalEvent()
        {
            m_signalEvent.Signal();
        }

        VkFence Fence::GetNativeFence() const
        {
            return m_nativeFence;
        }

        void Fence::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeFence), name.data(), VK_OBJECT_TYPE_FENCE, static_cast<Device&>(GetDevice()));
            }
        }

        RHI::ResultCode Fence::InitInternal(RHI::Device& baseDevice, RHI::FenceState initialState)
        {
            auto& device = static_cast<Device&>(baseDevice);
            DeviceObject::Init(baseDevice);

            VkFenceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            createInfo.pNext = nullptr;
            bool readyToWait = false;
            switch (initialState)
            {
            case RHI::FenceState::Reset:
                readyToWait = false;
                createInfo.flags = 0;
                break;
            case RHI::FenceState::Signaled:
                readyToWait = true;
                createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                break;
            default:
                return RHI::ResultCode::InvalidArgument;
            }

            const VkResult result = device.GetContext().CreateFence(device.GetNativeDevice(), &createInfo, nullptr, &m_nativeFence);
            AssertSuccess(result);

            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));
            m_signalEvent.SetValue(readyToWait);
            SetName(GetName());
            return RHI::ResultCode::Success;
        }

        void Fence::ShutdownInternal()
        {
            if (m_nativeFence != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroyFence(device.GetNativeDevice(), m_nativeFence, nullptr);
                m_nativeFence = VK_NULL_HANDLE;
            }
            // Signal any pending thread.
            m_signalEvent.Signal();
        }

        void Fence::SignalOnCpuInternal()
        {
            // Vulkan doesn't have an API to signal fences from CPU.
            // Because of this we need to use a queue to signal the fence.
            auto& device = static_cast<Device&>(GetDevice());
            device.GetCommandQueueContext().GetCommandQueue(RHI::HardwareQueueClass::Graphics).Signal(*this);
        }

        void Fence::WaitOnCpuInternal() const
        {
            // According to the standard we can't wait until the event (like VkSubmitQueue) happens first.
            m_signalEvent.Wait();
            auto& device = static_cast<Device&>(GetDevice());
            AssertSuccess(device.GetContext().WaitForFences(device.GetNativeDevice(), 1, &m_nativeFence, VK_FALSE, UINT64_MAX));
        }

        void Fence::ResetInternal()
        {
            m_signalEvent.SetValue(false);
            auto& device = static_cast<Device&>(GetDevice());
            AssertSuccess(device.GetContext().ResetFences(device.GetNativeDevice(), 1, &m_nativeFence));
        }

        RHI::FenceState Fence::GetFenceStateInternal() const
        {
            auto& device = static_cast<Device&>(GetDevice());
            VkResult result = device.GetContext().GetFenceStatus(device.GetNativeDevice(), m_nativeFence);
            switch (result)
            {
            case VK_SUCCESS:
                return RHI::FenceState::Signaled;
            case VK_NOT_READY:
                return RHI::FenceState::Reset;
            case VK_ERROR_DEVICE_LOST:
                AZ_Assert(false, "Device is lost.");
                break;
            default:
                AZ_Assert(false, "Fence state is unknown.");
                break;
            }
            return RHI::FenceState::Reset;
        }
    }
}

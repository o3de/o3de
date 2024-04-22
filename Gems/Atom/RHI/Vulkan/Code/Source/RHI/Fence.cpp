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
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<Fence> Fence::Create()
        {
            return aznew Fence();
        }

        FenceType Fence::GetFenceType() const
        {
            return m_fenceType;
        }

        VkFence Fence::GetNativeFence() const
        {
            AZ_Assert(m_fenceType == FenceType::Fence, "Vulkan Fence::GetNativeFence: Invalid type");
            return m_nativeFence;
        }

        void Fence::SignalEvent()
        {
            if (m_semaphoreHandle)
            {
                m_semaphoreHandle->SignalSemaphores(1);
            }
            switch (m_fenceType)
            {
            case FenceType::Fence:
                m_signalEvent.Signal();
                break;
            case FenceType::TimelineSemaphore:
                break; // Nothing to do as timeline semaphores can be waited for before they are submitted
            default:
                AZ_Assert(false, "Vulkan Fence::SignalEvent: Invalid fence type");
                break;
            }
        }

        VkSemaphore Fence::GetNativeSemaphore() const
        {
            AZ_Assert(m_fenceType == FenceType::TimelineSemaphore, "Vulkan Fence::GetNativeSemaphore: Invalid type");
            return m_nativeSemaphore;
        }

        uint64_t Fence::GetPendingValue() const
        {
            AZ_Assert(m_fenceType == FenceType::TimelineSemaphore, "Vulkan Fence::GetPendingValue: Invalid type");
            return m_pendingValue;
        }

        void Fence::SetSemaphoreHandle(AZStd::shared_ptr<SemaphoreTrackerHandle> semaphoreHandle)
        {
            m_semaphoreHandle = semaphoreHandle;
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

            m_fenceType = FenceType::Invalid;
            if (device.GetFeatures().m_signalFenceFromCPU)
            {
                VkSemaphoreTypeCreateInfo timelineCreateInfo{};
                timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
                timelineCreateInfo.pNext = nullptr;
                timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
                timelineCreateInfo.initialValue = 0;

                VkSemaphoreCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                createInfo.pNext = &timelineCreateInfo;
                createInfo.flags = 0;

                const VkResult result = device.GetContext().CreateSemaphore(
                    device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeSemaphore);
                AssertSuccess(result);

                RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));

                m_pendingValue = (initialState == RHI::FenceState::Signaled) ? 0 : 1;
                m_fenceType = FenceType::TimelineSemaphore;

                return RHI::ResultCode::Success;
            }
            else
            {
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

                const VkResult result =
                    device.GetContext().CreateFence(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeFence);
                AssertSuccess(result);

                RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(result));
                m_signalEvent.SetValue(readyToWait);
                SetName(GetName());
                m_fenceType = FenceType::Fence;
                return RHI::ResultCode::Success;
            }
        }

        void Fence::ShutdownInternal()
        {
            switch (m_fenceType)
            {
            case FenceType::Fence:
                {
                    if (m_nativeFence != VK_NULL_HANDLE)
                    {
                        auto& device = static_cast<Device&>(GetDevice());
                        device.GetContext().DestroyFence(device.GetNativeDevice(), m_nativeFence, VkSystemAllocator::Get());
                        m_nativeFence = VK_NULL_HANDLE;
                    }
                }
                break;
            case FenceType::TimelineSemaphore:
                {
                    if (m_nativeSemaphore != VK_NULL_HANDLE)
                    {
                        auto& device = static_cast<Device&>(GetDevice());
                        device.GetContext().DestroySemaphore(device.GetNativeDevice(), m_nativeSemaphore, VkSystemAllocator::Get());
                        m_nativeSemaphore = VK_NULL_HANDLE;
                    }
                }
                break;
            default:
                break;
            }
            // Signal any pending thread.
            SignalEvent();
        }

        void Fence::SignalOnCpuInternal()
        {
            switch (m_fenceType)
            {
            case FenceType::Fence:
                {
                    // Vulkan doesn't have an API to signal fences from CPU.
                    // Because of this we need to use a queue to signal the fence.
                    auto& device = static_cast<Device&>(GetDevice());
                    device.GetCommandQueueContext().GetCommandQueue(RHI::HardwareQueueClass::Graphics).Signal(*this);
                }
                break;
            case FenceType::TimelineSemaphore:
                {
                    VkSemaphoreSignalInfo signalInfo;
                    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
                    signalInfo.pNext = nullptr;
                    signalInfo.semaphore = m_nativeSemaphore;
                    signalInfo.value = m_pendingValue;

                    auto& device = static_cast<Device&>(GetDevice());
                    device.GetContext().SignalSemaphore(device.GetNativeDevice(), &signalInfo);
                    if (m_semaphoreHandle)
                    {
                        m_semaphoreHandle->SignalSemaphores(1);
                    }
                }
                break;
            default:
                AZ_Assert(false, "Vulkan Fence::SignalOnCpuInternal: Invalid fence type");
                break;
            }
        }

        void Fence::WaitOnCpuInternal() const
        {
            switch (m_fenceType)
            {
            case FenceType::Fence:
                {
                    // According to the standard we can't wait until the event (like VkSubmitQueue) happens first.
                    m_signalEvent.Wait();
                    auto& device = static_cast<Device&>(GetDevice());
                    AssertSuccess(device.GetContext().WaitForFences(device.GetNativeDevice(), 1, &m_nativeFence, VK_FALSE, UINT64_MAX));
                }
                break;
            case FenceType::TimelineSemaphore:
                {
                    VkSemaphoreWaitInfo waitInfo{};
                    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
                    waitInfo.pNext = nullptr;
                    waitInfo.flags = 0;
                    waitInfo.semaphoreCount = 1;
                    waitInfo.pSemaphores = &m_nativeSemaphore;
                    waitInfo.pValues = &m_pendingValue;

                    auto& device = static_cast<Device&>(GetDevice());
                    device.GetContext().WaitSemaphores(device.GetNativeDevice(), &waitInfo, AZStd::numeric_limits<uint64_t>::max());
                }
                break;
            default:
                AZ_Assert(false, "Vulkan Fence::WaitOnCpuInternal: Invalid fence type");
                break;
            }
        }

        void Fence::ResetInternal()
        {
            m_semaphoreHandle = {};
            switch (m_fenceType)
            {
            case FenceType::Fence:
                {
                    m_signalEvent.SetValue(false);
                    auto& device = static_cast<Device&>(GetDevice());
                    AssertSuccess(device.GetContext().ResetFences(device.GetNativeDevice(), 1, &m_nativeFence));
                }
                break;
            case FenceType::TimelineSemaphore:
                {
                    m_pendingValue++;
                }
                break;
            default:
                AZ_Assert(false, "Vulkan Fence::ResetInternal: Invalid fence type");
                break;
            }
        }

        RHI::FenceState Fence::GetFenceStateInternal() const
        {
            switch (m_fenceType)
            {
            case FenceType::Fence:
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
                break;
            case FenceType::TimelineSemaphore:
                {
                    auto& device = static_cast<Device&>(GetDevice());
                    uint64_t completedValue = 0;
                    device.GetContext().GetSemaphoreCounterValue(device.GetNativeDevice(), m_nativeSemaphore, &completedValue);
                    return (m_pendingValue <= completedValue) ? RHI::FenceState::Signaled : RHI::FenceState::Reset;
                }
                break;
            default:
                AZ_Assert(false, "Vulkan Fence::GetFenceStateInternal: Invalid fence type");
                return RHI::FenceState::Reset;
                break;
            }
        }
    }
}

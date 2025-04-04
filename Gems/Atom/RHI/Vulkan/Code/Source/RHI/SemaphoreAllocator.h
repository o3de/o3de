/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Object.h>
#include <Atom/RHI/ObjectPool.h>
#include <AzCore/std/parallel/mutex.h>
#include <RHI/BinarySemaphore.h>
#include <RHI/Device.h>
#include <RHI/TimelineSemaphore.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        namespace Internal
        {
            template<bool ForceBinarySemaphores>
            class SemaphoreFactory final : public RHI::ObjectFactoryBase<Semaphore>
            {
                using Base = RHI::ObjectFactoryBase<Semaphore>;
            public:
                struct Descriptor
                {
                    Device* m_device = nullptr;
                };

                void Init(const Descriptor& descriptor)
                {
                    m_descriptor = descriptor;
                }

                RHI::Ptr<Semaphore> CreateObject()
                {
                    RHI::Ptr<Semaphore> semaphore;
                    if (m_descriptor.m_device->GetFeatures().m_signalFenceFromCPU && !ForceBinarySemaphores)
                    {
                        semaphore = TimelineSemaphore::Create();
                    }
                    else
                    {
                        semaphore = BinarySemaphore::Create();
                    }
                    if (semaphore->Init(*m_descriptor.m_device) != RHI::ResultCode::Success)
                    {
                        AZ_Printf("Vulkan", "Failed to initialize Semaphore");
                        return nullptr;
                    }

                    return semaphore;
                }

                void ResetObject(Semaphore& semaphore)
                {
                    semaphore.Reset();
                }
                void ShutdownObject(Semaphore& semaphore, bool isPoolShutdown)
                {
                    AZ_UNUSED(semaphore);
                    AZ_UNUSED(isPoolShutdown);
                }
                bool CollectObject(Semaphore& semaphore)
                {
                    return semaphore.GetRecycleValue();
                }

            private:
                Descriptor m_descriptor;
            };

            template<bool ForceBinarySemaphores>
            struct SemaphoreAllocatorTraits : public RHI::ObjectPoolTraits
            {
                using ObjectType = Semaphore;
                using ObjectFactoryType = SemaphoreFactory<ForceBinarySemaphores>;
                using MutexType = AZStd::mutex;
            };
        }

        // Semaphore allocator using an ObjectPool that recycles the semaphores after they have been used.
        // Since we can't recycle vulkan semaphores we have to wait until they have been used (collect latency after
        // deallocation). Semaphores that will never be signaled or waited (e.g. because the swapchain was destroyed)
        // will not be recycled and they just be destroy during the collect phase.
        // We need a separate allocator for swapchains as Vulkan swapchains can only be synchronized with a binary semaphore:
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueuePresentKHR.html#VUID-vkQueuePresentKHR-pWaitSemaphores-03267
        using SemaphoreAllocator = RHI::ObjectPool<Internal::SemaphoreAllocatorTraits<false>>;
        using SwapChainSemaphoreAllocator = RHI::ObjectPool<Internal::SemaphoreAllocatorTraits<true>>;
    }
}

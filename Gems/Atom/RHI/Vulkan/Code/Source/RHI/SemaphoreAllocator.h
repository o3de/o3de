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

#include <AzCore/std/parallel/mutex.h>
#include <Atom/RHI/Object.h>
#include <Atom/RHI/ObjectPool.h>
#include <RHI/Semaphore.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        namespace Internal
        {
            class SemaphoreFactory final
                : public RHI::ObjectFactoryBase<Semaphore>
            {
                using Base = RHI::ObjectFactoryBase<Semaphore>;
            public:
                struct Descriptor
                {
                    Device* m_device = nullptr;
                };

                void Init(const Descriptor& descriptor);

                RHI::Ptr<Semaphore> CreateObject();

                void ResetObject(Semaphore& semaphore);
                void ShutdownObject(Semaphore& semaphore, bool isPoolShutdown);
                bool CollectObject(Semaphore& semaphore);

            private:
                Descriptor m_descriptor;
            };

            struct SemaphoreAllocatorTraits : public RHI::ObjectPoolTraits
            {
                using ObjectType = Semaphore;
                using ObjectFactoryType = SemaphoreFactory;
                using MutexType = AZStd::mutex;
            };
        }

        // Semaphore allocator using an ObjectPool that recycles the semaphores after they have been used.
        // Since we can't recycle vulkan semaphores we have to wait until they have been used (collect latency after
        // deallocation). Semaphores that will never be signaled or waited (e.g. because the swapchain was destroyed)
        // will not be recycled and they just be destroy during the collect phase.
        using SemaphoreAllocator = RHI::ObjectPool<Internal::SemaphoreAllocatorTraits>;
    }
}

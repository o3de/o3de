/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/base.h>
#include <RHI/SemaphoreAllocator.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        namespace Internal
        {
            void SemaphoreFactory::Init(const Descriptor& descriptor)
            {
                m_descriptor = descriptor;
            }

            RHI::Ptr<Semaphore> SemaphoreFactory::CreateObject()
            {
                RHI::Ptr<Semaphore> semaphore = Semaphore::Create();
                if (semaphore->Init(*m_descriptor.m_device) != RHI::ResultCode::Success)
                {
                    AZ_Printf("Vulkan", "Failed to initialize Semaphore");
                    return nullptr;
                }

                return semaphore;
            }

            void SemaphoreFactory::ResetObject(Semaphore& semaphore)
            {
                semaphore.ResetSignalEvent();
            }

            void SemaphoreFactory::ShutdownObject(Semaphore& semaphore, bool isPoolShutdown)
            {
                AZ_UNUSED(semaphore);
                AZ_UNUSED(isPoolShutdown);                
            }

            bool SemaphoreFactory::CollectObject(Semaphore& semaphore)
            {
                return semaphore.GetRecycleValue();
            }
        }           
    }
}

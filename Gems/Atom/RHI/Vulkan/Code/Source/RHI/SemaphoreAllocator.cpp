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
#include "Atom_RHI_Vulkan_precompiled.h"
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

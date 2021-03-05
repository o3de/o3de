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

#include <Atom/RHI/ResourcePool.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class CommandList;

        class ResourcePoolResolver
            : public RHI::ResourcePoolResolver
        {
        public:
            AZ_RTTI(ResourcePoolResolver, "F5FDB9DF-0C3A-4AC8-9E86-9ADABCD1267F", RHI::ResourcePoolResolver);

            ResourcePoolResolver(Device& device)
                : m_device(device)
            {}

            /// Called during compilation of the frame, prior to execution.
            virtual void Compile([[maybe_unused]] const RHI::HardwareQueueClass hardwareClass) {}

            /// Queues transition barriers at the beginning of a scope.
            virtual void QueuePrologueTransitionBarriers(CommandList&) {}

            /// Performs resolve-specific copy / streaming operations.
            virtual void Resolve(CommandList&) {}

            /// Queues transition barriers at the end of a scope.
            virtual void QueueEpilogueTransitionBarriers(CommandList&) {}

            /// Called at the end of the frame after execution.
            virtual void Deactivate() {}

            /// Called when a resource from the pool is being Shutdown
            virtual void OnResourceShutdown([[maybe_unused]] const RHI::Resource& resource) {}

            Device& GetDevice() const { return m_device; }

        protected:
            Device& m_device;
        };
    }
}

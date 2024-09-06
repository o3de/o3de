/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceResourcePool.h>

namespace AZ::WebGPU
{
    class Device;
    class CommandList;

    class ResourcePoolResolver
        : public RHI::ResourcePoolResolver
    {
    public:
        AZ_RTTI(ResourcePoolResolver, "{D5F82462-9CA9-47F0-9BE1-15E05AB57DC9}", RHI::ResourcePoolResolver);

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
        virtual void OnResourceShutdown([[maybe_unused]] const RHI::DeviceResource& resource) {}

        Device& GetDevice() const { return m_device; }

    protected:
        Device& m_device;
    };
}

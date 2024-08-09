/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/DeviceResourcePool.h>

namespace AZ
{
    namespace Metal
    {
        class CommandList;
        class ResourcePoolResolver
            : public RHI::ResourcePoolResolver
        {
        public:
            AZ_RTTI(ResourcePoolResolver, "{CCA67B06-218B-4727-BD86-A754DCBCA200}", RHI::ResourcePoolResolver);

            ResourcePoolResolver(Device& device)
                : m_device(device)
            {}
            
            /// Called during compilation of the frame, prior to execution.
            virtual void Compile() {}

            /// Performs resolve-specific copy / streaming operations.
            virtual void Resolve(CommandList&) const {}

            /// Called at the end of the frame after execution.
            virtual void Deactivate() {}
            
            /// Called when a resource from the pool is being Shutdown
            virtual void OnResourceShutdown(const RHI::DeviceResource& resource) {}

            Device& GetDevice() const { return m_device; }

        protected:
            Device& m_device;
        };
    }
}

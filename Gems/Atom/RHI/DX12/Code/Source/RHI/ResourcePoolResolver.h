/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameEventBus.h>

namespace AZ
{
    namespace DX12
    {
        class CommandList;
        class Scope;

        class ResourcePoolResolver
            : public RHI::ResourcePoolResolver
        {
        public:
            AZ_RTTI(ResourcePoolResolver, "{5AA79078-BDB6-4D2C-96EA-BD7AF9E78EEC}", RHI::ResourcePoolResolver);
            virtual ~ResourcePoolResolver() = default;
            
            /// Called during compilation of the frame, prior to execution.
            virtual void Compile([[maybe_unused]] Scope& scope) {}

            /// Queues transition barriers at the beginning of a scope.
            virtual void QueuePrologueTransitionBarriers(CommandList&) {}

            /// Performs resolve-specific copy / streaming operations.
            virtual void Resolve(CommandList&) const {}

            /// Queues transition barriers at the end of a scope.
            virtual void QueueEpilogueTransitionBarriers(CommandList&) const {}

            /// Called at the end of the frame after execution.
            virtual void Deactivate() {}

            /// Called when a resource from the pool is being Shutdown
            virtual void OnResourceShutdown([[maybe_unused]] const RHI::DeviceResource& resource) {}
        };
    }
}

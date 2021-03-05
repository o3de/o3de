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
            virtual void QueuePrologueTransitionBarriers(CommandList&) const {}

            /// Performs resolve-specific copy / streaming operations.
            virtual void Resolve(CommandList&) const {}

            /// Queues transition barriers at the end of a scope.
            virtual void QueueEpilogueTransitionBarriers(CommandList&) const {}

            /// Called at the end of the frame after execution.
            virtual void Deactivate() {}

            /// Called when a resource from the pool is being Shutdown
            virtual void OnResourceShutdown([[maybe_unused]] const RHI::Resource& resource) {}
        };
    }
}

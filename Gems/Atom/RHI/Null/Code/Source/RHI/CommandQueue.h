/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CommandQueue.h>
#include <RHI/CommandList.h>


namespace AZ
{
    namespace Null
    {
        class CommandQueue final
            : public RHI::CommandQueue
        {
            using Base = RHI::CommandQueue;
        public:
            AZ_CLASS_ALLOCATOR(CommandQueue, AZ::SystemAllocator);
            AZ_RTTI(CommandQueue, "{5835D7F8-B991-44F2-8EE9-533C3B33B8AB}", Base);
            
            using Descriptor = RHI::CommandQueueDescriptor;
            static RHI::Ptr<CommandQueue> Create();
        
            //////////////////////////////////////////////////////////////////////////
            // RHI::CommandQueue
            void ExecuteWork([[maybe_unused]] const RHI::ExecuteWorkRequest& request) override {}
            void WaitForIdle() override {}
            //////////////////////////////////////////////////////////////////////////

        private:
            CommandQueue() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::CommandQueue
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::CommandQueueDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            void* GetNativeQueue() override { return nullptr;}
            //////////////////////////////////////////////////////////////////////////

        };
    }
}

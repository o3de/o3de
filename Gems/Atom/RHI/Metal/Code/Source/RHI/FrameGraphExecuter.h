/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <RHI/CommandList.h>
#include <RHI/Metal.h>
#include <RHI/Scope.h>
#include <Atom/RHI.Reflect/Metal/PlatformLimitsDescriptor.h>

namespace AZ
{
    namespace Metal
    {
        class FrameGraphExecuter final
            : public RHI::FrameGraphExecuter
        {
            using Base = RHI::FrameGraphExecuter;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuter, AZ::SystemAllocator, 0);

            static RHI::Ptr<FrameGraphExecuter> Create();

            Device& GetDevice() const;
        private:
            FrameGraphExecuter();

            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameGraphExecuter
            RHI::ResultCode InitInternal(const RHI::FrameGraphExecuterDescriptor& descriptor) override;
            void ShutdownInternal() override;
            void BeginInternal(const RHI::FrameGraph& frameGraph) override;
            void ExecuteGroupInternal(RHI::FrameGraphExecuteGroup& group) override;
            void EndInternal() override {}
            
            CommandQueueContext* m_commandQueueContext = nullptr;
            FrameGraphExecuterData m_frameGraphExecuterData;

        };
    }
}

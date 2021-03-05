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

#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI.Reflect/DX12/PlatformLimitsDescriptor.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/CommandList.h>
#include <RHI/Fence.h>

namespace AZ
{
    namespace DX12
    {
        class CommandQueueContext;

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
            //////////////////////////////////////////////////////////////////////////

            // Forces all scopes to issue a dedicated scope group with one command list.
            void BeginInternalDebug(const RHI::FrameGraph& frameGraph);

            CommandQueueContext* m_commandQueueContext = nullptr;

            const RHI::ScopeId m_mergedScopeId{"Merged"};
            FrameGraphExecuterData m_frameGraphExecuterData;
        };
    }
}

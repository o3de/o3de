/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/FrameGraphExecuteGroupHandlerBase.h>
#include <RHI/RenderPassBuilder.h>

namespace AZ
{
    namespace Vulkan
    {
        //! Handler for a list of ExecuteGroups that are part of the same graph group.
        //! All the execute groups share the same renderpass and each of the groups correspond to
        //! a subpass of the renderpass. Each execute group uses one or more secondary command list.
        //! The handler owns the primary command list that will execute the secondary command list of each execute group.
        class FrameGraphExecuteGroupHandler final
            : public FrameGraphExecuteGroupHandlerBase
        {
            using Base = FrameGraphExecuteGroupHandlerBase;

        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupHandler, AZ::SystemAllocator, 0);

            FrameGraphExecuteGroupHandler() = default;
            ~FrameGraphExecuteGroupHandler() = default;

        private:
            //////////////////////////////////////////////////////////////////////////
            // FrameGraphExecuteGroupHandlerBase
            RHI::ResultCode InitInternal(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups) override;
            void EndInternal() override;
            //////////////////////////////////////////////////////////////////////////

            void EmitScopeBarriers(CommandList& commandList, Scope::BarrierSlot slot) const;
            
            // Clear scope's UAVs
            void ProcessClearRequests(CommandList& commandList) const;

            // Resets the RHI QueryPools and its RHI Queries that are used for the scopes of the FrameGraphExecuteGroup.
            void ResetQueryPools(CommandList& commandList) const;

            // RenderPassContext that is shared by all groups.
            RenderPassContext m_renderPassContext;
        };
    }
}

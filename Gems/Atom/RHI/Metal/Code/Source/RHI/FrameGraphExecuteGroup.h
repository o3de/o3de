/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/FrameGraphExecuteGroupBase.h>

namespace AZ
{
    namespace Metal
    {
    class Scope;
        class FrameGraphExecuteGroup final
            : public FrameGraphExecuteGroupBase
        {
            using Base = FrameGraphExecuteGroupBase;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroup, AZ::PoolAllocator, 0);

            FrameGraphExecuteGroup() = default;
            ~FrameGraphExecuteGroup();
            
            void Init(
                Device& device,
                const Scope& scope,
                AZ::u32 commandListCount,
                RHI::JobPolicy globalJobPolicy,
                uint32_t groupIndex);

        private:
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameSchedulerExecuteGroup
            void BeginInternal() override;
            void EndInternal() override;
            void BeginContextInternal(RHI::FrameGraphExecuteContext& context, uint32_t contextIndex) override;
            void EndContextInternal(RHI::FrameGraphExecuteContext& context, uint32_t contextIndex) override;
            //////////////////////////////////////////////////////////////////////////

            struct SubEncoderData
            {
                CommandList* m_commandList;
                id <MTLCommandEncoder> m_subRenderEncoder;
            };
            
            //Container to hold commandlist and render encoder to be used per contextId.
            AZStd::vector<SubEncoderData > m_subRenderEncoders;
            
            const Scope* m_scope = nullptr;
            NSString* m_cbLabel = nullptr;
        };
    }
}

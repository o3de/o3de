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

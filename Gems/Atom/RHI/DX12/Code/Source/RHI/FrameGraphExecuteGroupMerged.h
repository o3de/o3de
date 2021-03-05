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
    namespace DX12
    {
        class FrameGraphExecuteGroupMerged final
            : public FrameGraphExecuteGroupBase
        {
            using Base = FrameGraphExecuteGroupBase;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupMerged, AZ::PoolAllocator, 0);

            FrameGraphExecuteGroupMerged() = default;

            void Init(Device& device, AZStd::vector<const Scope*>&& scopes, const RHI::ScopeId& mergedScopeId);

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::ExecuteContextGroupBase
            void BeginInternal() override;
            void BeginContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;
            void EndContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;
            void EndInternal() override;
            //////////////////////////////////////////////////////////////////////////

            int32_t m_lastCompletedScope = -1;
            AZStd::vector<const Scope*> m_scopes;

            RHI::ScopeId m_mergedScopeId;
        };
    }
}

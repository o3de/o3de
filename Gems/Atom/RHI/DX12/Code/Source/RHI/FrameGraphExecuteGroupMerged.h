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
    namespace DX12
    {
        class FrameGraphExecuteGroupMerged final
            : public FrameGraphExecuteGroupBase
        {
            using Base = FrameGraphExecuteGroupBase;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupMerged, AZ::PoolAllocator);

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

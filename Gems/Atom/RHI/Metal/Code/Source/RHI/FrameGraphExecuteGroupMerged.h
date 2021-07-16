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
        class FrameGraphExecuteGroupMerged final
            : public FrameGraphExecuteGroupBase
        {
            using Base = FrameGraphExecuteGroupBase;
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupMerged, AZ::PoolAllocator, 0);

            FrameGraphExecuteGroupMerged() = default;

            void Init(Device& device, AZStd::vector<const Scope*>&& scopes, uint32_t groupIndex);

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::FrameGraphExecuteGroup
            void BeginInternal() override;
            void BeginContextInternal(RHI::FrameGraphExecuteContext& context, AZ::u32 contextIndex) override;
            void EndContextInternal(RHI::FrameGraphExecuteContext& context, AZ::u32 contextIndex) override;
            void EndInternal() override;
            //////////////////////////////////////////////////////////////////////////

            AZ::s32 m_lastCompletedScope = -1;
            AZStd::vector<const Scope*> m_scopes;
            NSString* m_cbLabel = nullptr;
        };

    }
}
